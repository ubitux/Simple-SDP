#include <stdio.h>
#include <memory.h>
#include "sdp.h"

#ifdef _MSC_VER
static const char * const files[] = {
    "../samples/1.sdp",
    "../samples/2.sdp",
    "../samples/3.sdp",
    "../samples/rfc-example.sdp",
};
#else
static const char * const files[] = {
    "./samples/1.sdp",
    "./samples/2.sdp",
    "./samples/3.sdp",
    "./samples/rfc-example.sdp",
};
#endif // _MSC_VER


int main(void)
{
    size_t i;

    for(i = 0; i < sizeof(files) / sizeof(*files); i++)
    {
        FILE* pFile;
        errno_t fopenError = fopen_s(&pFile, files[i], "rb");
        struct sdp_payload *sdp;
        char payload[1024 + 1];
        size_t n;

        if(0 != fopenError)
        {
            perror("fopen_s");
            return 1;
        }

        fseek(pFile, 0, SEEK_END);

        long  fileSize = ftell(pFile);

        rewind(pFile);

        memset(payload, 0x0, 1025);

        n = fread(payload, fileSize, 1, pFile);
        if(n < 0)
        {
            perror("fread");
            return 1;
        }
        // payload[n] = 0;
        sdp = sdp_parse(payload);
        printf("[%s]\n", files[i]);
        sdp_dump(sdp);
        printf("\n");
        sdp_destroy(sdp);
    }
    return 0;
}
