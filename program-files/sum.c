#define SIZE 1024
int _start() {
  int sum = 0;
  for (int i = 0; i < SIZE; i++)
    sum += 2;
  return sum;
}
