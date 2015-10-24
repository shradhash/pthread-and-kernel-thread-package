#include <stdio.h>
#include "cpu_info.h"
#define NUMCORE 4
#define NUMHTHREAD 2
int get_num_of_cores()
{
  int num_cores;
  char cores[2];
  FILE *fp;
  
  num_cores = NUMCORE;

  fp = popen("lscpu | grep Core | cut -f 2 -d ':' | xargs", "r");
  if (fp == NULL)
  {
     num_cores = NUMCORE;
  }

  if (fgets(cores, sizeof(cores), fp) == NULL)
  {
     num_cores = NUMCORE;
  }
  else
  {
     num_cores = atoi(cores);
  }

  return num_cores;
}

int get_num_of_hyperthreads()
{
  int num_hyperthreads;
  char hthread[2];
  FILE *fp;

  num_hyperthreads = NUMHTHREAD;

  fp = popen("lscpu | grep Thread | cut -f 2 -d ':' | xargs", "r");
  if (fp == NULL)
  {
     num_hyperthreads = NUMHTHREAD;
  }

  if (fgets(hthread, sizeof(hthread), fp) == NULL)
  {
     num_hyperthreads = NUMHTHREAD;
  }
  else
  {
     num_hyperthreads = atoi(hthread);
  }

  return num_hyperthreads;
}

