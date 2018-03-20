
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

#define N_SAMPLES 20000

Coords samplePoint[N_SAMPLES];
int sampleDistance[N_SAMPLES];

void TestDistanceFunction(const char* functionName,
    Coords_DistanceFunction distanceFunction)
{
  Coords origin = { RANDOM_RANGE / 2, RANDOM_RANGE / 2 };
  double sum;
  for (int i=0; i < N_SAMPLES; ++i)
  {
    sampleDistance[i] = distanceFunction(origin, samplePoint[i]);
    sum += sampleDistance[i];
  }
  double mean = sum / N_SAMPLES;
  double sumOfSquaredDeviations = 0;
  double sumOfDeviations = 0;
  for (int i=0; i < N_SAMPLES; ++i)
  {
    double sampleDeviation = sampleDistance[i] - mean;
    sumOfSquaredDeviations += sampleDeviation * sampleDeviation;
    sumOfDeviations += sampleDeviation;
  }
  double standardDeviation = sqrt(sumOfSquaredDeviations);
  double meanDeviation = sumOfDeviations / N_SAMPLES;
  printf("%s: Mean=%g; SD=%g; MeanDev=%g\n",
      functionName, mean, standardDeviation, meanDeviation);
}

void TestDistanceFunctions()
{
  for (int i=0; i < N_SAMPLES; ++i)
  {
    Coords randomPoint = { randomInt(), randomInt() };
    samplePoint[i] = randomPoint;
  }
  TestDistanceFunction("Approx", Coords_ApproxDist);
  TestDistanceFunction("Exact", Coords_ExactDist);
  TestDistanceFunction("Float", Coords_FloatDist);
}

int main(int argc, char** argv)
{
  assert(RAND_MAX > RANDOM_RANGE);
  srand(1);
  TestDistanceFunctions();
  return 0;
}

