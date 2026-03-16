#include <cstdint>
#include <iostream>

namespace
{

int compute(int value)
{
  const int doubled = value * 2;
  return doubled + 7;
}

} // namespace

int main()
{
  std::cout << compute(5) << '\n';
  return 0;
}
