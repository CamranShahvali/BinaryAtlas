#include <cstdint>
#include <iostream>

namespace
{

int accumulate(int limit)
{
  int total = 0;
  for (int index = 0; index < limit; ++index)
  {
    total += index;
  }
  return total;
}

} // namespace

int main()
{
  std::cout << accumulate(8) << '\n';
  return 0;
}
