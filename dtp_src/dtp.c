#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

typedef struct dtb_info {
  uint32_t total_size;
  uint32_t addr_start;
  uint32_t addr_stop;
  char *new_file;
} dtb;

static void die(char *message) {
  fprintf(stderr, message);
  exit(EXIT_FAILURE);
}

static void usage(char *argv0) {
  printf("Example: \n\t%s -i kernel_dtb\n", argv0);
  exit(EXIT_FAILURE);
}

static void check_arg(int argc, char *argv[], char *dtb_file) {
  char optstring[] = "i:";
  int c, index = 0;
  int input_exist = 0;
  struct option options[] = {
      {"input", 1, NULL, 'i'},
      {0, 0, 0, 0},
  };
  while ((c = getopt_long(argc, argv, optstring, options, &index)) != -1) {
    switch (c) {
      case 'i':
        strcpy(dtb_file, optarg);
        input_exist = 1;
        break;
      default:
        usage(argv[0]);
        break;
    }
  }
  if(input_exist!=1){
      usage(argv[0]);
  }
}

static void *memcpy_(void *dest, void *src, size_t size) {
  uint8_t *d = (uint8_t *)dest;
  uint8_t *s = (uint8_t *)src;
  int i = 0;
  while (size-- > 0) {
    d[size] = s[i];
    i++;
  }
  return dest;
}

int main(int argc, char *argv[]) {
  char *file_ = malloc(sizeof(char) * 100);
  char *filename = malloc(sizeof(char) * 100);
  check_arg(argc,argv,file_);

  struct stat filebuff;
  stat(file_, &filebuff);
  long filesize = filebuff.st_size;

  uint8_t buff[4];  // read 4 hex 1 time
  uint8_t head[4] = {0xD0, 0x0D, 0xFE, 0xED};
  uint8_t buff_c;
  FILE *fp = fopen(file_, "rb");
  if (fp == NULL) {
    fclose(fp);
    die("open dtb file failed\n");
  }

  fgets(buff, 5, fp);  // compare with head
  int ret = memcmp(&buff, &head, sizeof(buff));
  if (ret != 0) {
    fclose(fp);
    die("not valid dtb file\n");
  }

  dtb info;
  int new_dtb_count = 0;
  long cur_pos = ftell(fp);
  while (cur_pos < filesize) {
    fgets(buff, 5, fp);
    memcpy_(&info.total_size, &buff, 4);  // dtb file big endian, wtf
    printf("dtb block: %d, block size:%d\n", new_dtb_count, info.total_size);
    info.addr_start = cur_pos - 4;
    info.addr_stop = info.addr_start + info.total_size;

    fseek(fp, info.addr_start, SEEK_SET);
    sprintf(filename, "%s-%d", file_, new_dtb_count);
    printf("%s\n", filename);
    FILE *new_dtb = fopen(filename, "wb");
    for (uint32_t i = 0; i < info.total_size; i++) {
      buff_c = fgetc(fp);
      fputc(buff_c, new_dtb);
    }
    fclose(new_dtb);

    fseek(fp, info.addr_stop + 4, SEEK_SET);  // move fp to next dtb block size
    cur_pos = ftell(fp);
    new_dtb_count++;
  }

  fclose(fp);
  return 0;
}