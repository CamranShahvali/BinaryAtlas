__attribute__((noinline)) int dispatch_case(int value)
{
  switch (value)
  {
  case 0:
    return 10;
  case 1:
    return 11;
  case 2:
    return 12;
  case 3:
    return 13;
  case 4:
    return 14;
  case 5:
    return 15;
  case 6:
    return 16;
  case 7:
    return 17;
  case 8:
    return 18;
  case 9:
    return 19;
  default:
    return -1;
  }
}

int main(void)
{
  volatile int seed = 5;
  return dispatch_case(seed);
}
