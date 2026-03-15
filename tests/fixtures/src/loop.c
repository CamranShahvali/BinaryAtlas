__attribute__((noinline)) int accumulate(int count)
{
  int total = 0;
  for (int index = 0; index < count; ++index)
  {
    if ((index & 1) == 0)
    {
      total += index;
    }
    else
    {
      total -= index;
    }
  }
  return total;
}

int main(void)
{
  return accumulate(12);
}
