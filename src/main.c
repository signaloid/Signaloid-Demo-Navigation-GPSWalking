/*
 *	Authored 2021, Vasileios Tsoutsouras.
 *
 *	All rights reserved.
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *	*	Redistributions of source code must retain the above
 *		copyright notice, this list of conditions and the following
 *		disclaimer.
 *	*	Redistributions in binary form must reproduce the above
 *		copyright notice, this list of conditions and the following
 *		disclaimer in the documentation and/or other materials
 *		provided with the distribution.
 *	*	Neither the name of the author nor the names of its
 *		contributors may be used to endorse or promote products
 *		derived from this software without specific prior written
 *		permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *	POSSIBILITY OF SUCH DAMAGE.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uxhw.h>
#include <unistd.h>

#define EARTH_RADIUS			6371000 /* metres */
#define PI						3.1415926

enum {
	kBenchmarkModesDirectComputation            = 1,
	kBenchmarkInputBufferStrLen 				= 1280,
	kBenchmarkActivityStrLen 					= 32,
	kBenchmarkSamplesPerDistribution			= 32,
	kBenchmarkSamplesPerFileLine				= 64,
	kBenchmarkDeltaTimeThresholdMs				= 1000,
	kBenchmarkDefaultNumberOfPositions			= 16,
	kBenchmarkUseAltitudeInSpeedEstimation		= 0,
	kBenchmarkUpperBoundOfExaminedUserPositions	= -1 /*	-1 stands for unlimited	*/
};

double calcDistanceUsingGPSCoordinatesWithoutAltitude(double waypoint1_Lat, double waypoint1_Lon, double waypoint2_Lat, double waypoint2_Lon);
double calcDistanceUsingGPSCoordinates(double waypoint1_Lat, double waypoint1_Lon, double waypoint1_Alt, double waypoint2_Lat, double waypoint2_Lon, double waypoint2_Alt);
void   usage(FILE *);

int
main(int argc, char *argv[])
{
	int    mode = 0;
	double gpsLatSamples[kBenchmarkSamplesPerFileLine + 1];
	double gpsLonSamples[kBenchmarkSamplesPerFileLine + 1];
	double gpsAltSamples[kBenchmarkSamplesPerFileLine + 1];
	/*
	 *	gpsSpeedSamples gets discarded
	 */
	double gpsSpeedSamples;
	char   inputBuffer[kBenchmarkInputBufferStrLen];
	char * fileName = "sensoringData_gps_clean_user1_walking_driving-uncertainT-64.csv";
	size_t fileNameLength;
	char   activityStr[kBenchmarkActivityStrLen];
	FILE * inputUserFile;
	int    performSpeedCalc = 0;
	int    id;
	int    userName;
	int    activityId;
	int    positionsCnt = 0;
	double timeStamp;
	double gpsBearing;
	double gpsAccuracy;
	double gpsLatIncrement;
	double gpsLonIncrement;
	double gpsAltIncrement;
	double gpsStartLatDistr;
	double gpsStartLonDistr;
	double gpsStartAltDistr;
	double gpsFinishLatDistr;
	double gpsFinishLonDistr;
	double gpsFinishAltDistr;
	double gpsPreviousLat = 0.0;
	double gpsPreviousLon = 0.0;
	double gpsPreviousAlt = 0.0;
	double timeStampInit = -1;
	double deltaTimeStamp;
	double gpsCalcSpeedDistr[kBenchmarkDefaultNumberOfPositions];
	double gpsCalcSpeedParticle;
	int    numOfSamplesPerDistribution = kBenchmarkSamplesPerDistribution;
	int    c;

	while ((c = getopt(argc, argv, "f:s:m:h")) != -1)
	{
		switch (c)
		{
		case 'f':
			fileNameLength = strlen(optarg);
			fileName= (char *)malloc(fileNameLength + 1);
			if (fileName == NULL)
			{
				fprintf(stderr, "memory allocation failed.\n");
				exit(EXIT_FAILURE);
			}
			strncpy(fileName, optarg, fileNameLength + 1);
			break;
		case 's':
			numOfSamplesPerDistribution = atoi(optarg);
			break;
		case 'm':
			/*
			 *	Get mode from arguments.
			 */
			if (strcmp("1", optarg) == 0)
			{
				mode = kBenchmarkModesDirectComputation;
			}
			break;
		case 'h':
			usage(stdout);
			exit(EXIT_SUCCESS);
		default:
			fprintf(stderr, "arguments malformed\n");
			usage(stderr);
			exit(EXIT_FAILURE);
		}
	}

	printf("Input filename: %s\n", fileName);
	inputUserFile = fopen(fileName, "r");
	if(inputUserFile == NULL)
	{
		fprintf(stderr, "Error opening %s\n", fileName);
		exit(EXIT_FAILURE);
	}

	/*
	 *	Ignore first line
	 */
	fscanf(inputUserFile, "%s", inputBuffer);

	while (fscanf(inputUserFile, "%s\n", inputBuffer) != EOF)
	{
		if (positionsCnt == kBenchmarkUpperBoundOfExaminedUserPositions)
		{
			break;
		}

		sscanf(inputBuffer, "%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%s\n", 
			&id, &userName, &timeStamp, &gpsLatIncrement, &gpsLonIncrement, &gpsAltIncrement,
			&gpsSpeedSamples, &gpsBearing, &gpsAccuracy, &activityId, activityStr);

		/*
		 *	Current GPS point (incremental)
		 */
		gpsPreviousLat += gpsLatIncrement;
		gpsPreviousLon += gpsLonIncrement;
		gpsPreviousAlt += gpsAltIncrement;

		/*
		 *	Get initial point
		 */
		if (timeStampInit < 0)
		{
			timeStampInit = timeStamp;
			gpsLatSamples[0] = gpsPreviousLat;
			gpsLonSamples[0] = gpsPreviousLon;

			if (kBenchmarkUseAltitudeInSpeedEstimation)
			{
				gpsAltSamples[0] = gpsPreviousAlt;
			}

			/*
			 *	Read generated Lat samples
			 */
			for (int samplesCnt=1; samplesCnt<kBenchmarkSamplesPerFileLine; samplesCnt++)
			{
				fscanf(inputUserFile, "%lf,", &gpsLatIncrement);
				gpsLatSamples[samplesCnt] = gpsLatSamples[0] + gpsLatIncrement;
			}
			fscanf(inputUserFile, "%lf\n", &gpsLatIncrement);
			gpsLatSamples[kBenchmarkSamplesPerFileLine] = gpsLatSamples[0] + gpsLatIncrement;

			/*
			 *	Read generated Lon samples
			 */
			for (int samplesCnt=1; samplesCnt<kBenchmarkSamplesPerFileLine; samplesCnt++)
			{
				fscanf(inputUserFile, "%lf,", &gpsLonIncrement);
				gpsLonSamples[samplesCnt] = gpsLonSamples[0] + gpsLonIncrement;
			}
			fscanf(inputUserFile, "%lf\n", &gpsLonIncrement);
			gpsLonSamples[kBenchmarkSamplesPerFileLine] = gpsLonSamples[0] + gpsLonIncrement;

			/*
			 *	Read generated Alt samples
			 */
			if (kBenchmarkUseAltitudeInSpeedEstimation)
			{
				for (int samplesCnt=1; samplesCnt<kBenchmarkSamplesPerFileLine; samplesCnt++)
				{
					fscanf(inputUserFile, "%lf,", &gpsAltIncrement);
					gpsAltSamples[samplesCnt] = gpsAltSamples[0] + gpsAltIncrement;
				}
				fscanf(inputUserFile, "%lf\n", &gpsAltIncrement);
				gpsAltSamples[kBenchmarkSamplesPerFileLine] = gpsAltSamples[0] + gpsAltIncrement;
			}

			if (mode != kBenchmarkModesDirectComputation)
			{
				gpsStartLatDistr = UxHwDoubleDistFromSamples(gpsLatSamples, numOfSamplesPerDistribution);
				gpsStartLonDistr = UxHwDoubleDistFromSamples(gpsLonSamples, numOfSamplesPerDistribution);

				if (kBenchmarkUseAltitudeInSpeedEstimation)
				{
					gpsStartAltDistr = UxHwDoubleDistFromSamples(gpsAltSamples, numOfSamplesPerDistribution);
				}
			}
			else
			{
				gpsStartLatDistr = 0.0;
				gpsStartLonDistr = 0.0;
				if (kBenchmarkUseAltitudeInSpeedEstimation)
				{
					gpsStartAltDistr = 0.0;
				}

				/*
				 *	Calculate mean value of input samples
				 */
				for (int i=0; i<numOfSamplesPerDistribution; i++)
				{
					gpsStartLatDistr += gpsLatSamples[i];
					gpsStartLonDistr += gpsLonSamples[i];
					if (kBenchmarkUseAltitudeInSpeedEstimation)
					{
						gpsStartAltDistr += gpsAltSamples[i];
					}
				}
				gpsStartLatDistr /= numOfSamplesPerDistribution;
				gpsStartLonDistr /= numOfSamplesPerDistribution;
				if (kBenchmarkUseAltitudeInSpeedEstimation)
				{
					gpsStartAltDistr /= numOfSamplesPerDistribution;
				}
			}
		}
		/*
		 *	Get point after threshold ms
		 */
		else if (timeStamp - timeStampInit > kBenchmarkDeltaTimeThresholdMs)
		{
			/*
			 *	Get initial point
			 */
			gpsLatSamples[0] = gpsPreviousLat;
			gpsLonSamples[0] = gpsPreviousLon;
			if (kBenchmarkUseAltitudeInSpeedEstimation)
			{
				gpsAltSamples[0] = gpsPreviousAlt;
			}

			/*
			 *	Read generated Lat samples
			 */
			for (int samplesCnt=1; samplesCnt<kBenchmarkSamplesPerFileLine; samplesCnt++)
			{
				fscanf(inputUserFile, "%lf,", &gpsLatIncrement);
				gpsLatSamples[samplesCnt] = gpsLatSamples[0] + gpsLatIncrement;
			}
			fscanf(inputUserFile, "%lf\n", &gpsLatIncrement);
			gpsLatSamples[kBenchmarkSamplesPerFileLine] = gpsLatSamples[0] + gpsLatIncrement;

			/*
			 *	Read generated Lon samples
			 */
			for (int samplesCnt=1; samplesCnt<kBenchmarkSamplesPerFileLine; samplesCnt++)
			{
				fscanf(inputUserFile, "%lf,", &gpsLonIncrement);
				gpsLonSamples[samplesCnt] = gpsLonSamples[0] + gpsLonIncrement;
			}
			fscanf(inputUserFile, "%lf\n", &gpsLonIncrement);
			gpsLonSamples[kBenchmarkSamplesPerFileLine] = gpsLonSamples[0] + gpsLonIncrement;

			/*
			 *	Read generated Alt samples
			 */
			if (kBenchmarkUseAltitudeInSpeedEstimation)
			{
				for (int samplesCnt=1; samplesCnt<kBenchmarkSamplesPerFileLine; samplesCnt++)
				{
					fscanf(inputUserFile, "%lf,", &gpsAltIncrement);
					gpsAltSamples[samplesCnt] = gpsAltSamples[0] + gpsAltIncrement;
				}
				fscanf(inputUserFile, "%lf\n", &gpsAltIncrement);
				gpsAltSamples[kBenchmarkSamplesPerFileLine] = gpsAltSamples[0] + gpsAltIncrement;
			}

			if (mode != kBenchmarkModesDirectComputation)
			{
				gpsFinishLatDistr = UxHwDoubleDistFromSamples(gpsLatSamples, numOfSamplesPerDistribution);
				gpsFinishLonDistr = UxHwDoubleDistFromSamples(gpsLonSamples, numOfSamplesPerDistribution);
				if (kBenchmarkUseAltitudeInSpeedEstimation)
				{
					gpsFinishAltDistr = UxHwDoubleDistFromSamples(gpsAltSamples, numOfSamplesPerDistribution);
				}
			}
			else
			{
				gpsFinishLatDistr = 0.0;
				gpsFinishLonDistr = 0.0;
				if (kBenchmarkUseAltitudeInSpeedEstimation)
				{
					gpsFinishAltDistr = 0.0;
				}

				/*
				 *	Calculate mean value of input samples
				 */
				for (int i=0; i<numOfSamplesPerDistribution; i++)
				{
					gpsFinishLatDistr += gpsLatSamples[i];
					gpsFinishLonDistr += gpsLonSamples[i];
					if (kBenchmarkUseAltitudeInSpeedEstimation)
					{
						gpsFinishAltDistr += gpsAltSamples[i];
					}
				}
				gpsFinishLatDistr /= numOfSamplesPerDistribution;
				gpsFinishLonDistr /= numOfSamplesPerDistribution;
				gpsFinishAltDistr /= numOfSamplesPerDistribution;
			}
			performSpeedCalc = 1;
		}
		else
		{
			/*
			 *	Discard the extra particle points
			 */
			fscanf(inputUserFile, "%s\n", inputBuffer);
			fscanf(inputUserFile, "%s\n", inputBuffer);
		}

		if (performSpeedCalc == 1)
		{
			printf("gpsStartLatDistr = %f\n", gpsStartLatDistr);
			printf("gpsStartLonDistr = %f\n", gpsStartLonDistr);

			if (kBenchmarkUseAltitudeInSpeedEstimation)
			{
				printf("gpsStartAltDistr = %f\n", gpsStartAltDistr);
			}

			printf("gpsFinishLatDistr = %f\n", gpsFinishLatDistr);
			printf("gpsFinishLonDistr = %f\n", gpsFinishLonDistr);

			if (kBenchmarkUseAltitudeInSpeedEstimation)
			{
				printf("gpsFinishAltDistr = %f\n", gpsFinishAltDistr);
			}

			deltaTimeStamp = timeStamp - timeStampInit;

			if (!kBenchmarkUseAltitudeInSpeedEstimation)
			{
				gpsCalcSpeedDistr[positionsCnt] = calcDistanceUsingGPSCoordinatesWithoutAltitude(
									gpsStartLatDistr,
									gpsStartLonDistr,
									gpsFinishLatDistr,
									gpsFinishLonDistr
								) / deltaTimeStamp;
				printf("Position %d: Mean value of speed estimation(w/o altitude): %f deltaTimeStamp: %f\n\n", positionsCnt, gpsCalcSpeedDistr[positionsCnt], deltaTimeStamp);
			}
			else
			{
				gpsCalcSpeedDistr[positionsCnt] = calcDistanceUsingGPSCoordinates(
									gpsStartLatDistr,
									gpsStartLonDistr,
									gpsStartAltDistr,
									gpsFinishLatDistr,
									gpsFinishLonDistr,
									gpsFinishAltDistr
								) / deltaTimeStamp;
				printf("Position %d: Mean value of speed estimation(with altitude): %f deltaTimeStamp: %f\n\n", positionsCnt, gpsCalcSpeedDistr[positionsCnt], deltaTimeStamp);
			}

			if (!kBenchmarkUseAltitudeInSpeedEstimation)
			{
				gpsCalcSpeedParticle = calcDistanceUsingGPSCoordinatesWithoutAltitude(
									UxHwDoubleNthMode(gpsStartLatDistr, 1),
									UxHwDoubleNthMode(gpsStartLonDistr, 1),
									UxHwDoubleNthMode(gpsFinishLatDistr, 1),
									UxHwDoubleNthMode(gpsFinishLonDistr, 1)
								) / deltaTimeStamp;
				printf("Position %d: Speed estimation using mean values (w/o altitude): %f deltaTimeStamp: %f\n\n", positionsCnt, gpsCalcSpeedParticle, deltaTimeStamp);
			}
			else
			{
				gpsCalcSpeedParticle = calcDistanceUsingGPSCoordinates(
									UxHwDoubleNthMode(gpsStartLatDistr, 1),
									UxHwDoubleNthMode(gpsStartLonDistr, 1),
									UxHwDoubleNthMode(gpsStartAltDistr, 1),
									UxHwDoubleNthMode(gpsFinishLatDistr, 1),
									UxHwDoubleNthMode(gpsFinishLonDistr, 1),
									UxHwDoubleNthMode(gpsFinishAltDistr, 1)
								) / deltaTimeStamp;
				printf("Position %d: Speed estimation using mean values (with altitude): %f deltaTimeStamp: %f\n\n", positionsCnt, gpsCalcSpeedParticle, deltaTimeStamp);
			}

			/*
			 *	Remove that to always maintain the initial position as the starting point
			 */
			gpsStartLatDistr = gpsFinishLatDistr;
			gpsStartLonDistr = gpsFinishLonDistr;

			if (kBenchmarkUseAltitudeInSpeedEstimation)
			{
				gpsStartAltDistr = gpsFinishAltDistr;
			}

			timeStampInit = timeStamp;
			performSpeedCalc = 0;
			positionsCnt++;
		}
	}

	fclose(inputUserFile);

	return EXIT_SUCCESS;
}


double
calcDistanceUsingGPSCoordinatesWithoutAltitude(double waypoint1_Lat, double waypoint1_Lon, double waypoint2_Lat, double waypoint2_Lon)
{
	double distanceDistr;

	waypoint1_Lat = (waypoint1_Lat / 180) * PI;
	waypoint2_Lat = (waypoint2_Lat / 180) * PI;
	waypoint1_Lon = (waypoint1_Lon / 180) * PI;
	waypoint2_Lon = (waypoint2_Lon / 180) * PI;

	double deltaLat = waypoint2_Lat - waypoint1_Lat;
	double deltaLon = waypoint2_Lon - waypoint1_Lon;
	double a = (sin(deltaLat/2) * sin(deltaLat/2)) + (sin(deltaLon/2) * sin(deltaLon/2) * cos(waypoint1_Lat) * cos(waypoint2_Lat));

	/*
	 *	If a contains negative values then c and eventually distanceDistr will have nan support positions.
	 *	Negative latitude and longitute are valid. Negative latitudes represent the southern hemisphere, and negative longitudes represent the western hemisphere.
	 *	However this does not mean that the way that we derive negative values from an initial GPS position is valid.
	 *	It can include a value that is in the southern hemisphere and part of the distribution shows it also in the northern hemisphere.
	 */
	double c = 2 * atan(sqrt(a) / sqrt(1-a));
	distanceDistr = EARTH_RADIUS * c;

	return distanceDistr;
}


double
calcDistanceUsingGPSCoordinates(double waypoint1_Lat, double waypoint1_Lon, double waypoint1_Alt, double waypoint2_Lat, double waypoint2_Lon, double waypoint2_Alt)
{
	double distanceDistr;

	waypoint1_Lat = (waypoint1_Lat / 180) * PI;
	waypoint2_Lat = (waypoint2_Lat / 180) * PI;
	waypoint1_Lon = (waypoint1_Lon / 180) * PI;
	waypoint2_Lon = (waypoint2_Lon / 180) * PI;

	distanceDistr = sqrt(pow(EARTH_RADIUS+waypoint1_Alt, 2) + pow(EARTH_RADIUS+waypoint2_Alt, 2) - 2*(EARTH_RADIUS+waypoint1_Alt) * (EARTH_RADIUS+waypoint2_Alt) *
						(sin(waypoint1_Lat) * sin(waypoint2_Lat) + cos(waypoint1_Lat) * cos(waypoint2_Lat) * cos(waypoint1_Lon-waypoint2_Lon)));

	return distanceDistr;
}

void
usage(FILE * out)
{
	fprintf(out,
	        "Usage: -f <file> -s <#sample> -m <mode: 1 for explicit computation, 0 for "
	        "implicit uncertainty tracking>\n");
}
