#include <cstdint>
#include <iostream>

namespace
{

int classify(int value)
{
  if (value < 0)
  {
    return -1;
  }
  if (value == 0)
  {
    return 0;
  }
  return value > 10 ? 2 : 1;
}

} // namespace

int main()
{
  std::cout << classify(12) << '\n';
  return 0;
}
