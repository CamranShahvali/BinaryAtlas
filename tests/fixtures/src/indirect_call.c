typedef int (*handler_t)(int);

__attribute__((noinline)) int plus_three(int value)
{
  return value + 3;
}

__attribute__((noinline)) int invoke(handler_t handler, int value)
{
  return handler(value);
}

int main(void)
{
  return invoke(plus_three, 39);
}
