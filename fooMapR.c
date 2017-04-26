#include <stdio.h>
#include <unistd.h>
#include <hbase/hbase.h>
#include <pthread.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <jni.h>
#include "JNIFoo.h"

#define CHECK_RC_RETURN(rc)          \
  do {                               \
    if (rc) {                        \
      return rc;                     \
    }                                \
  } while (0);

#define CHECK_RC_RETURN1(rc)          \
  do {                               \
    if (rc) {                        \
      ret = env->NewStringUTF("Not Done...");	\
      return ret;                     \
    }                                \
  } while (0);

char cldbs[1024] = "127.0.0.1:7222";
const char *tableName = "/tmp/testInteractive";
const char *family1 = "Id";
const char *col1_1  = "I";
const char *family2 = "Name";
const char *col2_1  = "First";
const char *col2_2  = "Last";
const char *family3 = "Address";
const char *col3_1  = "City";

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
uint64_t count = 0;
bool clientDestroyed;

struct RowBuffer {
  std::vector<char *> allocedBufs;

  RowBuffer() {
    allocedBufs.clear();
  }

  ~RowBuffer() {
    while (allocedBufs.size() > 0) {
      char *buf = allocedBufs.back();
      allocedBufs.pop_back();
      delete [] buf;
    }
  }

  char *getBuffer(uint32_t size) {
    char *newAlloc = new char[size];
    allocedBufs.push_back(newAlloc);
    return newAlloc;
  }
};

void read_result(hb_result_t result)
{
  if (!result) {
    return;
  }
  
  size_t cellCount = 0;
  hb_result_get_cell_count(result, &cellCount);


  for (size_t i = 0; i < cellCount; ++i) {
    const hb_cell_t *cell;
    hb_result_get_cell_at(result, i, &cell);
    printf("%s:%s = %s\t", cell->family, cell->qualifier, cell->value);
  }

  if (cellCount == 0) {
    printf("----- NO CELLS -----");
  }
}

void cl_dsc_cb(int32_t err, hb_client_t client, void *extra)
{
  clientDestroyed = true;
}

void client_flush_cb(int32_t err, hb_client_t client, void *ctx)
{
  printf("Client flush callback invoked: %d\n", err);
}

void put_cb(int err, hb_client_t client, hb_mutation_t mutation,
            hb_result_t result, void *extra)
{
  if (err != 0) {
    printf("PUT CALLBACK called err = %d\n", err);
  }

  hb_mutation_destroy(mutation);
  pthread_mutex_lock(&mutex);
  count ++;
  pthread_mutex_unlock(&mutex);
  if (result) {
    const byte_t *key;
    size_t keyLen;
    hb_result_get_key(result, &key, &keyLen);
    printf("Row: %s\t", (char *)key);
    read_result(result);
    printf("\n");
    hb_result_destroy(result);
  }

  if (extra) {
    RowBuffer *rowBuf = (RowBuffer *)extra;
    delete rowBuf;
  }
}


void create_dummy_cell(hb_cell_t **cell,
                      const char *r, size_t rLen,
                      const char *f, size_t fLen,
                      const char *q, size_t qLen,
                      const char *v, size_t vLen)
{
  hb_cell_t *cellPtr = new hb_cell_t();

  cellPtr->row = (byte_t *)r;
  cellPtr->row_len = rLen;

  cellPtr->family = (byte_t *)f;
  cellPtr->family_len = fLen;

  cellPtr->qualifier = (byte_t *)q;
  cellPtr->qualifier_len = qLen;

  cellPtr->value = (byte_t *)v;
  cellPtr->value_len = vLen;

  *cell = cellPtr;
}

int test_put(hb_client_t client, int id)
{
  RowBuffer *rowBuf = new RowBuffer();
  hb_put_t put;
  int err = 0;

  char *buffer = rowBuf->getBuffer(1024);
  char *i      = rowBuf->getBuffer(1024);
  char *first  = rowBuf->getBuffer(1024);
  char *last   = rowBuf->getBuffer(1024);
  char *city   = rowBuf->getBuffer(1024);


  err = hb_put_create((byte_t *)buffer, strlen(buffer) + 1, &put);
  CHECK_RC_RETURN(err);

  hb_cell_t *cell;
  create_dummy_cell(&cell, buffer, 1024, family1, strlen(family1) + 1, col1_1, strlen(col1_1) + 1, i, strlen(i) + 1);
  err = hb_put_add_cell(put, cell);
  delete cell;
  CHECK_RC_RETURN(err);

  create_dummy_cell(&cell, buffer, 1024, family2, strlen(family2) + 1, col2_1, strlen(col2_1) + 1, first, strlen(first) + 1);
  err = hb_put_add_cell(put, cell);
  delete cell;
  CHECK_RC_RETURN(err);

  create_dummy_cell(&cell, buffer, 1024, family2, strlen(family2) + 1, col2_2, strlen(col2_2) + 1, last, strlen(last) + 1);
  err = hb_put_add_cell(put, cell);
  delete cell;
  CHECK_RC_RETURN(err);

  create_dummy_cell(&cell, buffer, 1024, family3, strlen(family3) + 1, col3_1, strlen(col3_1) + 1, city, strlen(city) + 1);
  err = hb_put_add_cell(put, cell);
  delete cell;
  CHECK_RC_RETURN(err);

  err = hb_mutation_set_table((hb_mutation_t)put, tableName, strlen(tableName));
  CHECK_RC_RETURN(err);

  err = hb_mutation_send(client, (hb_mutation_t)put, put_cb, rowBuf);
  CHECK_RC_RETURN(err);
  return err;
}

JNIEXPORT jstring JNICALL Java_JNIFoo_nativeFoo (JNIEnv *env, jobject obj,jstring connArg)
{
  const char *nativeString = env->GetStringUTFChars(connArg, JNI_FALSE);

  printf("%s",nativeString);
  fflush(stdout);
  env->ReleaseStringUTFChars(connArg, nativeString);
   
  hb_connection_t conn;
  int err = 0;
  jstring ret = 0;

  err = hb_connection_create(cldbs, NULL, &conn);
  CHECK_RC_RETURN1(err);

  hb_client_t client;
  err = hb_client_create(conn, &client);
  CHECK_RC_RETURN1(err);

  uint64_t numRows = 2;

  count = 0;
  for (uint64_t i = 0; i < numRows; ++i) {
    err = test_put(client, i);
    CHECK_RC_RETURN1(err);
  }

  hb_client_flush(client, client_flush_cb, NULL);
  uint64_t locCount;
  do {
    sleep (1);
    pthread_mutex_lock(&mutex);
    locCount = count;
    pthread_mutex_unlock(&mutex);
  } while (locCount < numRows);


  err = hb_client_destroy(client, cl_dsc_cb, NULL);
  CHECK_RC_RETURN1(err);

  sleep(1);
  err = hb_connection_destroy(conn);
  CHECK_RC_RETURN1(err);
  ret = env->NewStringUTF("Done...");
  return ret;

}


