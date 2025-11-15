#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

int compare(const void *left, const void *right);
void swap(int64_t *arr, unsigned long i, unsigned long j);
unsigned long partition(int64_t *arr, unsigned long start, unsigned long end);
int quicksort(int64_t *arr, unsigned long start, unsigned long end, unsigned long par_threshold);

int main(int argc, char **argv) {
  unsigned long par_threshold;
  if (argc != 3 || sscanf(argv[2], "%lu", &par_threshold) != 1) {
    fprintf(stderr, "Usage: parsort <file> <par threshold>\n");
    exit(1);
  }

  // open the file in read write mode so we can edit it
  int fd = open(argv[1], O_RDWR);
  if (fd < 0) {
    perror("open fail");
    exit(1);
  }

  // get the info about file size
  struct stat sb;
  if (fstat(fd, &sb) != 0) {
    perror("fstat fail");
    close(fd);
    exit(1);
  }

  // get num of elements in the file
  unsigned long file_size = sb.st_size;
  unsigned long num_elements = file_size / sizeof(int64_t);

  // map file into memory for shared access
  int64_t *arr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  if (arr == MAP_FAILED) {
    perror("mmap fail");
    exit(1);
  }

  // run the parallel quicksort
  int success = quicksort(arr, 0, num_elements, par_threshold);
  if (!success) {
    fprintf(stderr, "error: sorting failed\n");
    munmap(arr, file_size);
    exit(1);
  }

  // unmap file when done
  if (munmap(arr, file_size) != 0) {
    perror("munmap fail");
    exit(1);
  }

  return 0;
}

// comparator for qsort
int compare(const void *left, const void *right) {
  int64_t l = *(const int64_t *)left;
  int64_t r = *(const int64_t *)right;
  return (l > r) - (l < r);
}

// swaps 2 values in the array
void swap(int64_t *arr, unsigned long i, unsigned long j) {
  int64_t tmp = arr[i];
  arr[i] = arr[j];
  arr[j] = tmp;
}

// does the split step for quicksort
unsigned long partition(int64_t *arr, unsigned long start, unsigned long end) {
  assert(end > start);
  unsigned long len = end - start;
  assert(len >= 2);

  // pick middle pivot
  unsigned long pivot_index = start + (len / 2);
  int64_t pivot_val = arr[pivot_index];
  swap(arr, pivot_index, end - 1);

  unsigned long left_index = start;
  unsigned long right_index = start + (len - 2);

  while (left_index <= right_index) {
    if (arr[left_index] < pivot_val) { ++left_index; continue; }
    if (arr[right_index] >= pivot_val) { --right_index; continue; }
    swap(arr, left_index, right_index);
  }

  swap(arr, left_index, end - 1);
  return left_index;
}

// recursive quicksort using child processes
int quicksort(int64_t *arr, unsigned long start, unsigned long end, unsigned long par_threshold) {
  assert(end >= start);
  unsigned long len = end - start;
  if (len < 2) return 1;

  // if small, just do normal qsort
  if (len <= par_threshold) {
    qsort(arr + start, len, sizeof(int64_t), compare);
    return 1;
  }

  // do partition step
  unsigned long mid = partition(arr, start, end);

  // fork to do left side
  pid_t left_pid = fork();
  if (left_pid < 0) {
    // fork failed
    return 0;
  }
  if (left_pid == 0) {
    int ok = quicksort(arr, start, mid, par_threshold);
    _exit(ok ? 0 : 1);
  }

  // fork to do right side
  pid_t right_pid = fork();
  if (right_pid < 0) {
    // if fork fails, wait left child to avoid zombie
    int st;
    (void)waitpid(left_pid, &st, 0);
    return 0;
  }
  if (right_pid == 0) {
    int ok = quicksort(arr, mid + 1, end, par_threshold);
    _exit(ok ? 0 : 1);
  }

  // parent waits for both kids to finish
  int status_left = 0, status_right = 0;
  int wl = waitpid(left_pid, &status_left, 0);
  int wr = waitpid(right_pid, &status_right, 0);
  if (wl < 0 || wr < 0) return 0;

  // check if both finished ok
  int left_success = WIFEXITED(status_left) && (WEXITSTATUS(status_left) == 0);
  int right_success = WIFEXITED(status_right) && (WEXITSTATUS(status_right) == 0);

  return left_success && right_success;
}
