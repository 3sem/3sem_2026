"""Real two-process IPC tests; no mocks. Run with `make test`."""

import argparse
import json
import os
from pathlib import Path
import resource
import tempfile

from support import METHODS, fileMd5, parseFields, readLimits, transferFile


def limitOutput():
    resource.setrlimit(resource.RLIMIT_FSIZE, (4096, 4096))


def checkTransfer(method, source, output, buffer_size):
    result = transferFile(method, source, output, buffer_size)
    assert result.returncode == 0, (method, buffer_size, result.stderr)
    fields = parseFields(result.stdout)
    assert fields["method"] == method
    assert int(fields["bytes"]) == source.stat().st_size == output.stat().st_size
    assert int(fields["buffer"]) == buffer_size
    assert float(fields["seconds"]) > 0
    assert fileMd5(source) == fileMd5(output), (method, source.stat().st_size, buffer_size)
    # Independent byte-for-byte comparison for the small correctness cases.
    with source.open("rb") as first, output.open("rb") as second:
        while True:
            block = first.read(1024 * 1024)
            assert block == second.read(1024 * 1024)
            if not block:
                break


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--large", action="store_true", help="also transfer >4 GiB")
    parser.add_argument("--report", type=Path, help="save a JSON test summary")
    arguments = parser.parse_args()
    limits = readLimits()
    small = limits["message_limit"]
    medium = limits["fifo_capacity"]
    buffer_sizes = (small, medium, 1024 * 1024)
    cases = [(size, buffer_size) for buffer_size in buffer_sizes
             for size in (0, 1, buffer_size - 1, buffer_size, buffer_size + 1)]
    cases += [(10003, 1), (10003, 17), (3 * 1024 * 1024 + 137, small + 13),
              (3 * 1024 * 1024 + 137, 65537)]
    report = {"limits": limits, "cases_per_method": len(cases), "methods": {}}
    with tempfile.TemporaryDirectory(prefix="task3_tests_") as directory:
        work_dir = Path(directory)
        source = work_dir / "input with spaces.bin"
        output = work_dir / "output.bin"
        for method in METHODS:
            for size, buffer_size in cases:
                source.write_bytes(os.urandom(size))
                output.write_bytes(b"old data" * 100)
                checkTransfer(method, source, output, buffer_size)

            source.write_bytes(os.urandom(10003))
            original = source.read_bytes()
            hard_link = work_dir / "hard_link.bin"
            soft_link = work_dir / "soft_link.bin"
            os.link(source, hard_link)
            soft_link.symlink_to(source)
            for destination in (source, hard_link, soft_link):
                result = transferFile(method, source, destination, small)
                assert result.returncode != 0
                assert source.read_bytes() == original
            hard_link.unlink()
            soft_link.unlink()

            for invalid in ("0", "-1", "abc", "16777217", "99999999999999999999", "", " 1", "+1"):
                result = transferFile(method, source, output, invalid)
                assert result.returncode != 0, (method, invalid)
            for input_path, output_path in (
                (work_dir / "missing", output), (work_dir, output),
                (source, work_dir), (source, work_dir / "missing" / "output"),
            ):
                assert transferFile(method, input_path, output_path, small).returncode != 0

            # Receiver dies on a real kernel file-size limit. Parent must not hang.
            source.write_bytes(os.urandom(4 * 1024 * 1024 + 13))
            failed = transferFile(method, source, output, small,
                                  preexec_fn=limitOutput, timeout=8)
            assert failed.returncode != 0, method
            # The subsequent successful transfer also exercises recovery.
            checkTransfer(method, source, output, 1024 * 1024)
            report["methods"][method] = "passed"
            print(f"{method}: {len(cases)} integrity cases, invalid files/buffers, receiver failure: OK",
                  flush=True)

        if arguments.large:
            large_size = 2**32 + 12345
            with source.open("wb") as stream:
                stream.truncate(large_size)
                for offset in (0, 2**32 - 4096, large_size - 4096):
                    stream.seek(offset)
                    stream.write(os.urandom(4096))
            source_md5 = fileMd5(source)
            report["large"] = {"bytes": large_size, "input_md5": source_md5, "runs": []}
            for method in METHODS:
                result = transferFile(method, source, output, 1024 * 1024, timeout=600)
                assert result.returncode == 0, result.stderr
                output_md5 = fileMd5(output)
                assert source.stat().st_size == output.stat().st_size == large_size
                assert output_md5 == source_md5
                fields = parseFields(result.stdout)
                fields["output_md5"] = output_md5
                report["large"]["runs"].append(fields)
                print(f"{method}: >4 GiB, MD5 {output_md5}, {fields['seconds']} s: OK", flush=True)
    if arguments.report:
        arguments.report.parent.mkdir(parents=True, exist_ok=True)
        arguments.report.write_text(json.dumps(report, indent=2) + "\n")
    print("All tests passed.")


if __name__ == "__main__":
    main()
