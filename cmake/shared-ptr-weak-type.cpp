#include <memory>

int main( )
{
   std::shared_ptr< int > p;
   std::shared_ptr< int >::weak_type wp { p };

   return 0;
}
