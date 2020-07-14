#include <map>

int main( )
{
   std::map< int, int > m1;
   m1.insert({1, 1});

   std::map< int, int > m2;
   m2.insert(m1.extract(1));

   return 0;
}
