__attribute__((noinline)) int callee(int value)
{
  return value * 2;
}

__attribute__((noinline)) int caller(int value)
{
  return callee(value) + 1;
}

int main(void)
{
  return caller(21);
}
