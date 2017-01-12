#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include "coder.h"
#include "bitio.h"
#include <vector>
#include <iostream>
#include <fstream>

#define MCSTREAM
//#define STATISTIC_STREAM


using namespace std;


const int sizeStream = 1000000;
const int sizeMatrixForMC = 5;

char *nameStreamFile = "stream.txt";
char *nameEncodingStreamFile = "encodingStream.txt";
char *nameDecodingFile = "decodingStream.txt";

struct probability{
	char c;
	unsigned short int low;
	unsigned short int high;
};

void readStremFromFile(char *nameStreamFile, vector<char> &stream)
{
	FILE *file = fopen(nameStreamFile, "rb");
	unsigned char ch;

	for (int i = 0; i < sizeStream; i++)
	{
		fread(&ch, 1, sizeof(unsigned char), file);
		stream.push_back(ch);
	}

	fclose(file);
}

void getStreamWithMC()
{
	int maxIteration = 50;
	float randNumb;
	float countOfCol;
	bool stop;
	int count;
	int current;
	float comulP;

#ifndef STATIC_MATRIX

	float matrixMC[sizeMatrixForMC][sizeMatrixForMC] = { 0 };
	{
		for (int i = 0; i < sizeMatrixForMC; i++)
		{
			countOfCol = 0;

			for (int j = 0; j < (sizeMatrixForMC - 1); j++)
			{
				count = 0;
				stop = false;

				while (stop != true && count < maxIteration)
				{
					randNumb = (float)rand() / RAND_MAX;
					if (randNumb <= (1 - countOfCol))
					{
						matrixMC[i][j] = randNumb;
						countOfCol += randNumb;
						stop = true;
					}
					count++;
				}
			}

			matrixMC[i][sizeMatrixForMC - 1] = 1 - countOfCol;
		}
	}

#endif

#ifdef STATIC_MATRIX

	float matrixMC[5][5] =
	{ { 0.1, 0.25, 0.15, 0.3, 0.2 },
	{ 0.1, 0.25, 0.05, 0.3, 0.2 },
	{ 0.1, 0.25, 0.05, 0.3, 0.2 },
	{ 0.1, 0.25, 0.05, 0.3, 0.2 },
	{ 0.1, 0.25, 0.05, 0.3, 0.2 } };

#endif

	printf("\t\t\tMattrix for MC\n");

	for (int i = 0; i < sizeMatrixForMC; i++)
	{
		for (int j = 0; j < sizeMatrixForMC; j++)
		{
			printf("%f\t", matrixMC[
				i][j]);
		}
		printf("\n");
	}

	current = round(((float)rand() / RAND_MAX) * (sizeMatrixForMC - 1));

	FILE *file = fopen(nameStreamFile, "wb");
	char ch;
	for (int i = 0; i < sizeStream; i++)
	{
		comulP = 0;

		for (int j = 0; j < sizeMatrixForMC; j++)
		{
			comulP += matrixMC[current][j];
			randNumb = (float)rand() / RAND_MAX;

			if (randNumb < comulP)
			{
				current = j;
				ch = current + 1;
				fwrite(&ch, 1, sizeof(unsigned char), file);
				break;
			}

		}


	}

	fclose(file);
}

void getStatic(vector<char> &output)
{
	float p[10] = { 0.1, 0.052, 0.05, 0.11, 0.14, 0.15, 0.04, 0.2, 0.06, 0.08 };
	float temp[10];
	FILE *file = fopen(nameStreamFile, "wb");
	char ch;

	for (int i = 1; i < 10; i++)
	{
		p[i] += p[i - 1];
		temp[i] = 0;
	}

	for (int i = 0; i < sizeStream; i++)
	{
		float tmp = (float)rand() / RAND_MAX;

		for (int j = 0; j < 10; j++)
		{

			if (tmp < p[j])
			{
				temp[j]++;
				ch = j + 1;
				output.push_back(j + 1);
				fwrite(&ch, 1, sizeof(char), file);
				break;
			}
		}
	}

	fclose(file);
};

void error_exit(char *message)
{
	puts(message);
	exit(-1);
}

void convert_int_to_symbol(char c, SYMBOL *s, probability *pr, int alphabetSize)
{
	int i;

	i = 0;
	for (;;)
	{
		if (c == pr[i].c)
		{
			s->low_count = pr[i].low;
			s->high_count = pr[i].high;
			s->scale = pr[alphabetSize - 1].high;
			return;
		}
		if (pr[i].c == '\0')
			error_exit("Trying to encode a char not in the table");
		i++;
	}
}


char convert_symbol_to_int(unsigned int count, SYMBOL *s, probability *pr, int alphabetSize)
{
	int i;

	i = 0;
	for (;;)
	{
		if (count >= pr[i].low &&
			count < pr[i].high)
		{
			s->low_count = pr[i].low;
			s->high_count = pr[i].high;
			s->scale = pr[alphabetSize - 1].high;
			return(pr[i].c);
		}
		if (pr[i].c == '\0')
			error_exit("Failure to decode character");
		i++;
	}
}

void compress(probability **pr, int alphabetSize, vector<char> input, char *output_filename)
{
	int i;
	char c;
	SYMBOL s;
	FILE *compressed_file;

	compressed_file = fopen(output_filename, "wb");
	if (compressed_file == NULL)
		error_exit("Could not open output file");
	puts("Compressing...");
	
	initialize_output_bitstream();
	
	initialize_arithmetic_encoder();
	int currTable;
	c = input[0];
	convert_int_to_symbol(c, &s, pr[0], alphabetSize);
	encode_symbol(compressed_file, &s);
	currTable = (int)c - 1;
	for (i = 1;i<input.size();i++)
	{
		
		c = input[i];
		convert_int_to_symbol(c, &s, pr[currTable], alphabetSize);
		encode_symbol(compressed_file, &s);
		currTable = (int)c - 1;
		
	}
	flush_arithmetic_encoder(compressed_file);
	flush_output_bitstream(compressed_file);
	fclose(compressed_file);
}


void expand(probability **pr, int alphabetSize, char *input_filename, char *output_filename)
{
	FILE *compressed_file, *expand_file;
	SYMBOL s;
	char c;
	int count;
	ofstream decodeStream(nameDecodingFile, ios::out | ios::binary);
	compressed_file = fopen(input_filename, "rb");
	if (compressed_file == NULL)
		error_exit("Could not open output file");
	puts("Decoding...");
	initialize_input_bitstream();
	initialize_arithmetic_decoder(compressed_file);
	

	s.scale = pr[0][alphabetSize - 1].high;
	count = get_current_count(&s);
	c = convert_symbol_to_int(count, &s, pr[0], alphabetSize);
	decodeStream << c;
	remove_symbol_from_stream(compressed_file, &s);
	int currTable = (int)c - 1;

	for (;;)
	{
		
		s.scale = pr[currTable][alphabetSize - 1].high;
		count = get_current_count(&s);
		c = convert_symbol_to_int(count, &s, pr[currTable], alphabetSize);
		currTable = (int)c - 1;
		decodeStream << c;
		if (c<0)
			break;
		remove_symbol_from_stream(compressed_file, &s);
		
	}
	decodeStream.close();
}




void main()
{	
	vector <char> stream;

	int alphabetSize = 5;
	probability **pr = new probability *[alphabetSize + 1];
	
	for (int i = 0; i < alphabetSize + 1; i++)
	{
		pr[i] = new probability[alphabetSize + 1];
	}
	
	float **p = new float*[alphabetSize];
	
	for (int i = 0; i < alphabetSize; i++)
	{
		p[i] = new float[alphabetSize];
		for (int j = 0; j < alphabetSize; j++)
		{
			p[i][j] = 1;
		}
	}

#ifdef  MCSTREAM
	
	getStreamWithMC();
	readStremFromFile(nameStreamFile, stream);

#endif

#ifdef  STATISTIC_STREAM
	getStatic(stream);
#endif

	int currPos = 0;

	for (int i = 0; i < stream.size(); i++)
	{
		if (stream[i] > alphabetSize)
		{
			stream[i] = alphabetSize;
		}

		p[currPos][stream[i] - 1]++;
		currPos = stream[i] - 1;
	}

	
	for (int n = 0; n < alphabetSize; n++)
	{
		int accum = 0;
		for (int i = 0; i < alphabetSize; i++)
		{
			p[n][i] = ceil(p[n][i] / stream.size() * 10000);
			pr[n][i].c = (char)(i + 1);
			pr[n][i].low = accum;
			accum += p[n][i];
			pr[n][i].high = accum;
			cout << (int)pr[n][i].c << "\t\t" << pr[n][i].low << "\t\t" << pr[n][i].high << endl;
		}

		pr[n][alphabetSize].c = '\0';
		pr[n][alphabetSize].low = pr[n][alphabetSize - 1].low;
		pr[n][alphabetSize].high = pr[n][alphabetSize - 1].high + 1;
		
		cout << endl;
		cout << endl;
		
	}
	
	
	compress(pr, alphabetSize, stream, nameEncodingStreamFile);
	expand(pr, alphabetSize, nameEncodingStreamFile, nameDecodingFile);
}


