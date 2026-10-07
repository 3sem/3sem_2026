#include "common.h"
#include "command_parser.h"
#include "run_cmd.h"

int main(void)
{
    while (1) {
        char *string_cmd = ReadCmd();
        if (string_cmd == NULL) {
            printf("\n");
            break;
        }

        CommandLine *cline = InitCommandLine();
        if (cline == NULL) {
            fprintf(stderr, "failed to allocate CommandLine\n");
            free(string_cmd);
            return 1;
        }

        CmdError err = ParseCommandLine(string_cmd, cline);
        free(string_cmd);
        if (err != OK) {
            fprintf(stderr, "ParseCommandLine failed with error %d\n", err);
            FreeCommandLine(cline);
            continue;   
        }


        if (cline->cmd_count == 0) {
            FreeCommandLine(cline);
            continue;
        }

        #if 0
        PrintCommandLineTable(cline);
        #endif

        RunCmd(cline);
        FreeCommandLine(cline);
    }

    return 0;
}