__attribute__((noinline)) int choose_value(int value)
{
  if (value > 10)
  {
    return value - 3;
  }
  return value + 7;
}

int main(void)
{
  volatile int seed = 7;
  return choose_value(seed);
}
