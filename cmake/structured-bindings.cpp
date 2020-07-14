#include <tuple>

std::tuple< int, bool >
get_tuple( )
{
   return { 1, true };
}

int main( )
{
   const auto [i, b] =
      get_tuple();

   return 0;
}
