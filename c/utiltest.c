
#include "wandrix.h"

#define RANDOM_RANGE 4096
#define RANDOM_END ((RAND_MAX / RANDOM_RANGE) * RANDOM_RANGE)

int randomInt()
{
  int r;
  do {
    r = rand();
  } while (r >= RANDOM_END);
  return r % RANDOM_RANGE;
}

#define N_SAMPLES 100000000

Coords origin = { RANDOM_RANGE / 2, RANDOM_RANGE / 2 };
Coords samplePoint[N_SAMPLES];
int sampleDistance[N_SAMPLES];
int sampleRealDistance[N_SAMPLES];

static void TestDistanceFunction(const char* functionName,
    Coords_DistanceFunction distanceFunction)
{
  double sum = 0, sumOfErrors = 0, sumOfSquaredErrors = 0;
  Uint32 startTime = SDL_GetTicks();
  for (int i=0; i < N_SAMPLES; ++i)
  {
    sampleDistance[i] = distanceFunction(origin, samplePoint[i]);
    sum += sampleDistance[i];
    double error = abs(sampleDistance[i] - sampleRealDistance[i]);
    sumOfErrors += error;
    sumOfSquaredErrors += error * error;
  }
  Uint32 endTime = SDL_GetTicks();
  double mean = sum / N_SAMPLES;
  double meanError = sumOfErrors / N_SAMPLES;
  double standardError = sqrt(sumOfSquaredErrors / (N_SAMPLES-1));
  double sumOfSquaredDeviations = 0;
  double sumOfDeviations = 0;
  for (int i=0; i < N_SAMPLES; ++i)
  {
    double sampleDeviation = abs(sampleDistance[i] - mean);
    sumOfSquaredDeviations += sampleDeviation * sampleDeviation;
    sumOfDeviations += sampleDeviation;
  }
  double standardDeviation = sqrt(sumOfSquaredDeviations / (N_SAMPLES-1));
  double meanDeviation = sumOfDeviations / N_SAMPLES;
  printf("%s: TimeMs=%d; Mean=%g; SD=%g; MeanDev=%g; SE=%g; MeanErr=%g\n",
      functionName,
      (int)(endTime - startTime), mean,
      standardDeviation, meanDeviation,
      standardError, meanError);
  fflush(stdout);
}

void TestDistanceFunctions()
{
  for (int i=0; i < N_SAMPLES; ++i)
  {
    Coords randomPoint = { randomInt(), randomInt() };
    samplePoint[i] = randomPoint;
    sampleRealDistance[i] = Coords_FloatDist(origin, randomPoint);
    if (i < 10)
      printf("(%d,%d) ", randomPoint.x, randomPoint.y);
    if (i < 10 && (i+1) % 5 == 0)
      printf("\n");
  }
  TestDistanceFunction("Simple", Coords_SimpleApproxDist);
  TestDistanceFunction("Simple", Coords_SimpleApproxDist);
  TestDistanceFunction("Approx", Coords_ApproxDist);
  TestDistanceFunction("Approx", Coords_ApproxDist);
  TestDistanceFunction("Exact", Coords_ExactDist);
  TestDistanceFunction("Exact", Coords_ExactDist);
  TestDistanceFunction("Float", Coords_FloatDist);
  TestDistanceFunction("Float", Coords_FloatDist);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc, char** argv)
{
  assert(RAND_MAX > RANDOM_RANGE);
  srand(1);
  TestDistanceFunctions();
  return 0;
}

