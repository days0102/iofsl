#include "BMIAddr.hh"


namespace iofwdutil
{
   namespace bmi
   {


      std::ostream & operator << (std::ostream & out, const BMIAddr & a)
      {
         out << a.toString(); 
         return out; 
      }

      const char * BMIAddr::toString (bool unexp) const
      {
         const char * ret = BMI_addr_rev_lookup (addr_);
         if (ret)
            return ret; 

         if (!unexp)
            return 0; 

         // Lookup failed: address invalid or unexpected
         return BMI_addr_rev_lookup_unexpected (addr_); 
      }

   }
}
