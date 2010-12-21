#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "sdp.h"

static const char * const files[] = {
    "./samples/1.sdp",
    "./samples/2.sdp",
    "./samples/3.sdp",
    "./samples/rfc-example.sdp",
};

int main(void)
{
    size_t i;

    for (i = 0; i < sizeof(files) / sizeof(*files); i++) {
        int fd = open(files[i], O_RDONLY);
        struct sdp_payload *sdp;
        char payload[1024+1];
        int n;

        if (fd < 0) {
            perror("open");
            return 1;
        }

        n = read(fd, payload, sizeof(payload));
        if (n < 0) {
            perror("read");
            return 1;
        }
        payload[n] = 0;
        sdp = sdp_parse(payload);
        printf("[%s]\n", files[i]);
        sdp_dump(sdp);
        printf("\n");
        sdp_destroy(sdp);
    }
    return 0;
}
