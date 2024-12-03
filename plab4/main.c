#include <stdlib.h>
#include <pthread.h>

FILE* binFile;
FILE* textFile;

void *writer_thread(void *arg)
{
}

void *reader_thread(void *arg)
{
}

int main()
{
  binFile = fopen("sensor_data","rb");
  textFile = fopen("sensor_data_out.csv","a");

  pthread_t writer, reader1, reader2;

  pthread_create(&writer, NULL, writer_thread, NULL);
  pthread_create(&reader1, NULL, reader_thread, NULL);
  pthread_create(&reader2, NULL, reader_thread, NULL);

  pthread_join(writer, NULL);
  pthread_join(reader1, NULL);
  pthread_join(reader2, NULL);

  return 0;
}
