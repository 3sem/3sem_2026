"""Common helpers for correctness tests and the reproducible experiment."""

import hashlib
import os
from pathlib import Path
import shutil
import signal
import subprocess

PROJECT_DIR = Path(__file__).resolve().parents[1]
METHODS = ("shm", "mq", "fifo")


def runCommand(command, timeout=120, **kwargs):
    with subprocess.Popen(
        [str(argument) for argument in command],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        start_new_session=True,
        **kwargs,
    ) as process:
        try:
            stdout, stderr = process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.communicate(timeout=3)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.communicate()
            raise
        return subprocess.CompletedProcess(command, process.returncode, stdout, stderr)


def parseFields(text):
    return dict(item.split("=", 1) for item in text.strip().split())


def readLimits():
    result = runCommand([PROJECT_DIR / "ipc_info"])
    result.check_returncode()
    return {name: int(value) for name, value in parseFields(result.stdout).items()}


def checksumTool():
    if shutil.which("md5sum"):
        return "md5sum"
    if shutil.which("md5"):
        return "md5 -q"
    return "Python hashlib.md5"


def fileMd5(path):
    tool = checksumTool()
    if tool != "Python hashlib.md5":
        result = runCommand([*tool.split(), path])
        result.check_returncode()
        return result.stdout.split()[0].lower()
    digest = hashlib.md5()
    with Path(path).open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def transferFile(method, source, output, buffer_size, **kwargs):
    return runCommand(
        [PROJECT_DIR / f"{method}_transfer", source, output, buffer_size], **kwargs
    )
