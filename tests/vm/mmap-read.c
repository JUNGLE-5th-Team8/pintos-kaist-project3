/* Uses a memory mapping to read a file. */

#include <string.h>
#include <syscall.h>
#include "tests/vm/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

void test_main(void)
{
  char *actual = (char *)0x10000000;
  int handle;
  void *map;
  size_t i;

  // printf("tlqkf\n");
  CHECK((handle = open("sample.txt")) > 1, "open \"sample.txt\"");
  // printf("tlqkf2222222222222222\n");
  CHECK((map = mmap(actual, 4096, 0, handle, 0)) != MAP_FAILED, "mmap \"sample.txt\"");
  // printf("tlqkf333333333333333333\n");

  /* Check that data is correct. */
  if (memcmp(actual, sample, strlen(sample)))
    fail("read of mmap'd file reported bad data");

  // printf("tlqkf4444444444444444444\n");

  /* Verify that data is followed by zeros. */
  for (i = strlen(sample); i < 4096; i++)
    if (actual[i] != 0)
      fail("byte %zu of mmap'd region has value %02hhx (should be 0)",
           i, actual[i]);

  munmap(map);
  close(handle);
}
