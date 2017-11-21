#include "stdafx.h"
#include <cstdio>
#include <aurloader.h>
#include <string>

bool CStrEqual(const char* pA, const char* pB)
{
#if defined(_MSC_VER)
	return _stricmp(pA, pB) == 0;
#else
	return strcasecmp(pA, pB) == 0;
#endif
}

float* multiply(float matrix[12], float vector[3])
{
	float result[3] = { 0 };

	result[0] = matrix[0] * vector[0] + matrix[1] * vector[1] + matrix[2] * vector[2];
	result[1] = matrix[4] * vector[0] + matrix[5] * vector[1] + matrix[6] * vector[2];
	result[2] = matrix[8] * vector[0] + matrix[9] * vector[1] + matrix[10] * vector[2];
	
	return result;
}

int* exportDimensions(IAur* AuraInterface, FILE* OutputFile)
{
	int Dimensions[3];
	AuraInterface->GetDim(Dimensions);
	// xCount, yCount, zCount
	fprintf(OutputFile, "%i, %i, %i,\n", Dimensions[0], Dimensions[1], Dimensions[2]);
	return Dimensions;
}

void exportBBox(IAur* AuraInterface, FILE* OutputFile, int Dimensions[3])
{	
	float Transform[12];
	AuraInterface->GetObject2GridTransform(Transform);

	float Matrix[16] = { Transform[0] * Dimensions[0], Transform[1] * Dimensions[0], Transform[2] * Dimensions[0], 0.0f,
				Transform[3] * Dimensions[1], Transform[4] * Dimensions[1], Transform[5] * Dimensions[2], 0.0f,
				Transform[6] * Dimensions[2], Transform[7] * Dimensions[2], Transform[8] * Dimensions[2], 0.0f,
				Transform[9], Transform[10], Transform[11], 1.0f };

	float NrmMin[3] = { 0 };
	float NrmMax[3] = { 1 };

	float* Min = multiply(Matrix, NrmMin);
	float* Max = multiply(Matrix, NrmMax);

	// boundMinX, boundsMinY, boundsMinZ, boundsMaxX, boundsMaxY, boundsMaxZ
	fprintf(OutputFile, "%f, %f, %f,\n %f, %f, %f,\n", Min[0], Min[1], Min[2], Max[0], Max[1], Max[2]);
}

void exportVelocity(IAur* AuraInterface, FILE* OutputFile, int Dimensions[3])
{
	if (!AuraInterface->ChannelPresent(GridChannels::ChVx))
	{
		printf("Velocity channel not present.\n");
		return;
	}

	const float* VelocityX = AuraInterface->ExpandChannel(GridChannels::ChVx);
	const float* VelocityY = AuraInterface->ExpandChannel(GridChannels::ChVy);
	const float* VelocityZ = AuraInterface->ExpandChannel(GridChannels::ChVz);

	const int RowPitch = Dimensions[0];
	const int SlicePitch = Dimensions[0] * Dimensions[1];

	for (auto z = 0; z < Dimensions[2]; ++z)
		for (auto y = 0; y < Dimensions[1]; ++y)
			for (auto x = 0; x < Dimensions[0]; ++x)
			{
				const int Index = x + (y * RowPitch) + (z * SlicePitch);
				fprintf(OutputFile, "%f, %f, %f,\n", VelocityX[Index], VelocityY[Index], VelocityZ[Index]);
			}
}

bool process(std::string& inputFileName, std::string& outputFileName)
{
	IAur* AuraInterface = newIAur(inputFileName.c_str());
	if (AuraInterface == nullptr)
	{
		printf("AuraInterface was null\n");
		return false;
	}

	FILE* outputFile = fopen(outputFileName.c_str(), "w");
	if (!outputFile)
	{
		printf("Failed to create file %s.\n", outputFileName.c_str());
		deleteIAur(AuraInterface);
		return false;
	}

	int* Dimensions = exportDimensions(AuraInterface, outputFile);
	exportBBox(AuraInterface, outputFile, Dimensions);
	exportVelocity(AuraInterface, outputFile, Dimensions);

	deleteIAur(AuraInterface);

	return true;
}

int main(int argc, char** argv)
{
	std::string inputFile;
	std::string outputFile;

	for (auto i = 1; i < argc; i++)
	{
		const char *arg = argv[i];

		if (CStrEqual(arg, "-i"))
			inputFile = argv[++i];
		else if (CStrEqual(arg, "-o"))
			outputFile = argv[++i];
	}

	process(inputFile, outputFile);

    return 0;
}