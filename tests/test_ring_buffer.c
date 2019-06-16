#include <stdio.h>
#include <assert.h>
#include "../ring_buffer.h"

void test_ring_buffer_put(ring_buffer *rb);
void test_ring_buffer_get(ring_buffer *rb);
void test_ring_buffer_put_get(ring_buffer *rb);


int main()
{
   printf("----------------------- Started testing ring buffer -----------------------\n");

   unsigned char buffer_in[8] = {'a','b','c','d','e','f','g','h'};
   unsigned char buffer_out[8];

   ring_buffer *rb = 0;

   rb = ring_buffer_alloc(8u);

   if (rb == 0)
   {
      LOG (0,"failed to allocate ring buffer");
      return -1;
   }

   //Call test routine
   test_ring_buffer_put(rb);

   if (ring_buffer_get(rb, &buffer_out[0], 8u) != 8u)
   {
      LOG(0,"failed to get from the buffer");
   }

   int c = 0;

   for (c = 0; c < 8; c++)
   {
      LOG(0,"buffer_out [%c] = ", buffer_out[c]);
   }

   if (ring_buffer_put(rb,&buffer_in[0],8u)!=8u)
   {
      LOG(0,"failed to put some data into buffer");
   }

   test_ring_buffer_get(rb);

   test_ring_buffer_put_get(rb);

   ring_buffer_free(rb);
   
   printf("----------------------- Finished testing ring buffer -----------------------\n");
   return 0;
}

void
test_ring_buffer_put(ring_buffer* rb)
{
   LOG(0,"Testing ring_buffer_put" );

   unsigned char buffer[8] = {'a','b','c','d','e','f','g','h'};

   ring_buffer_reset(rb);

   assert(ring_buffer_put(rb, &buffer[0], 3u) == 3u);

   assert(ring_buffer_put(rb, &buffer[0], 1u) == 1u);

   assert(ring_buffer_put(rb, &buffer[0], 8u) == 8u);

   assert(ring_buffer_put(rb, &buffer[0], 0u) == 0u);

   assert(ring_buffer_put(rb, &buffer[0], 9u) == 8u);

   ring_buffer_reset(rb);

   assert(ring_buffer_put(rb, &buffer[0], 8u) == 8u);

}

void test_ring_buffer_get(ring_buffer* rb)
{
   LOG(0,"Testing ring_buffer_get" );

   unsigned char buffer_out[8];
   unsigned int len = 0u;

   if (ring_buffer_len(rb) > 0)
   {
      len = ring_buffer_len(rb);
      LOG(0,"current buffer length %d", len);

      //----------------------------- test get -----------------------------//

      assert(ring_buffer_get(rb, &buffer_out[0], 1u) == 1u);

      assert(buffer_out[0]=='a');

      assert(ring_buffer_len(rb)==7);

      //----------------------------- test get -----------------------------//

      assert(ring_buffer_get(rb, &buffer_out[0], 5u) == 5u);

      assert(buffer_out[0] == 'b');

      assert(buffer_out[4] == 'f');

      assert(ring_buffer_len(rb)==2);

      //----------------------------- test get -----------------------------//

      assert(ring_buffer_get(rb, &buffer_out[0], 0u) == 0u);

      len = ring_buffer_len(rb);

      LOG(0,"current buffer length %d", len);

      assert(ring_buffer_len(rb)==2);

      //----------------------------- test get -----------------------------//

      assert(ring_buffer_get(rb, &buffer_out[0], 3u) == 2u);

      assert(buffer_out[0] == 'g');

      assert(buffer_out[1] == 'h');

      assert (ring_buffer_len(rb)==0);

      //----------------------------- test get -----------------------------//

      assert(ring_buffer_get(rb, &buffer_out[0], 1u) == 0u);

      len = ring_buffer_len(rb);

      LOG(0,"current buffer length %d", len);

      assert(len==0);

   }else
   {
      LOG(0,"buffer is empty nothing to test");
   }
}

void test_ring_buffer_put_get(ring_buffer* rb)
{
   LOG(0,"Testing ring_buffer_put_get" );

   unsigned char buffer_out[8];
   unsigned char buffer_in[8] = {'a','b','c','d','e','f','g','h'};
   unsigned int len = 0u;


   len = ring_buffer_len(rb);
   LOG(0,"current buffer length %d", len);

   //----------------------------- test get -----------------------------//

   //Put complete array into the buffer, buffer is full after put
   assert(ring_buffer_put(rb, &buffer_in[0],8u)==8u);
   
   //Check size
   assert(ring_buffer_len(rb)==8u);

   //Get first 3 chars a,b,c
   assert(ring_buffer_get(rb, &buffer_out[0], 3u) == 3u);
   
   //Check size
   assert((len = ring_buffer_len(rb))==5);

   //Put 6 chars to generate buffer owerflow: i, j, k, l, m, n
   //Next get should start at 'g' at index 6
   unsigned char buffer_in2[8] = {'i','j','k','l','m','n','o','p'};
   assert(ring_buffer_put(rb, &buffer_in2[0],6u)==6u);

   //Get 4 chars
   assert(ring_buffer_get(rb, &buffer_out[0], 4u) == 4u);
   assert((len = ring_buffer_len(rb))==4u);
   assert(buffer_out[0]=='g');
   assert(buffer_out[1]=='h');
   assert(buffer_out[2]=='i');
   assert(buffer_out[3]=='j');

   //Get 4 chars
   assert(ring_buffer_get(rb, &buffer_out[0], 4u) == 4u);
   assert((len = ring_buffer_len(rb))==0u);
   assert(buffer_out[0]=='k');
   assert(buffer_out[1]=='l');
   assert(buffer_out[2]=='m');
   assert(buffer_out[3]=='n');
   
  
   assert(ring_buffer_get(rb, &buffer_out[0], 6u) == 0u);
   assert((len = ring_buffer_len(rb))==0u);

   LOG(0,"current buffer length %d", len);
}
