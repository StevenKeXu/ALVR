#include <gtest/gtest.h>

extern "C" {
#include "../../ALVR-common/reedsolomon/rs.h"
};
#include "../../alvr_server/Listener.h"
#include "../../ALVR-common/packet_types.h"

TEST(rs_test, init) {
	reed_solomon_init();
	reed_solomon *rs = reed_solomon_new(10, 10);
	ASSERT_TRUE(rs != NULL);
	reed_solomon_release(rs);
}

TEST(rs_test, encode1) {
	reed_solomon_init();
	int data_shards = 5;
	int parity_shards = 10;
	int shards = data_shards + parity_shards;
	reed_solomon *rs = reed_solomon_new(data_shards, parity_shards);

	int block_size = 5;
	char *p = new char[block_size * shards];
	memcpy(p, "lie5ic3zei3osaete4wuuwooB", block_size * data_shards);
	char **p2 = new char *[shards];
	for (int i = 0; i < shards; i++) {
		p2[i] = p + i * block_size;
	}
	reed_solomon_encode(rs, (unsigned char **)p2, shards, block_size);
	ASSERT_EQ(memcmp(p, "lie5ic3zei3osaete4wuuwooB", block_size * data_shards), 0);

	unsigned char *marks = new unsigned char[shards];
	
	for (int i = 0; i < shards; i++) {
		marks[i] = 0;
	}
	memset(p, 111, 5 * 8);
	marks[0] = 1;
	marks[1] = 1;
	marks[2] = 1;
	marks[3] = 1;
	marks[4] = 1;
	marks[5] = 1;
	marks[6] = 1;
	marks[7] = 1;
	marks[8] = 1;
	//marks[9] = 1;
	//marks[10] = 1;
	marks[11] = 1;
	int ret = reed_solomon_reconstruct(rs, (unsigned char **)p2, marks, shards, block_size);
	ASSERT_EQ(ret, 0);
	ASSERT_EQ(memcmp(p, "lie5ic3zei3osaete4wuuwooB", block_size * data_shards), 0);
	reed_solomon_release(rs);
}


TEST(rs_test, encode2) {
	srand(1);
	reed_solomon_init();
	int data_shards = 90;
	int parity_shards = 10;
	int shards = data_shards + parity_shards;
	reed_solomon *rs = reed_solomon_new(data_shards, parity_shards);

	for (int j = 0; j < 100; j++) {

		int size = 1000 * 9 * (rand() % 100);
		char *p = new char[size * shards / data_shards];
		char *orig = new char[size * shards / data_shards];
		for (int i = 0; i < size; i++) {
			orig[i] = rand() % 50 + 'A';
		}

		char **p2 = new char *[shards];
		unsigned char *marks = new unsigned char[shards];

		memset(orig + size, 0, size * parity_shards / data_shards);
		memcpy(p, orig, size * shards / data_shards);

		int block_size = size / data_shards;
		ASSERT_EQ(size % data_shards, 0);

		for (int i = 0; i < shards; i++) {
			p2[i] = p + i * block_size;
		}
		reed_solomon_encode(rs, (unsigned char **)p2, shards, block_size);
		ASSERT_EQ(memcmp(p, orig, size), 0);

		for (int i = 0; i < shards; i++) {
			marks[i] = 0;
		}

		// Simulate drop N shards
		int N = rand() % 20;
		for (int i = 0; i < N; i++) {
			int n = rand() % shards;
			if (marks[n] != 0) {
				i--;
				continue;
			}
			marks[n] = 1;
			memset(p + n * block_size, 0, block_size);
		}

		int ret = reed_solomon_reconstruct(rs, (unsigned char **)p2, marks, shards, block_size);
		if (N <= parity_shards) {
			ASSERT_EQ(ret, 0);
			ASSERT_EQ(memcmp(p, orig, size), 0);
		}
		else {
			ASSERT_NE(ret, 0);
			ASSERT_NE(memcmp(p, orig, size), 0);
		}
		delete[] marks;
		delete[] orig;
		delete[] p;
		delete[] p2;
	}
	reed_solomon_release(rs);
}


TEST(rs_test, packets) {
	// fecPercentage=0
	ASSERT_EQ(CalculateFECShardPackets(1, 0), 1);
	ASSERT_EQ(CalculateFECShardPackets(2, 0), 1);
	ASSERT_EQ(CalculateFECShardPackets(1 * ALVR_MAX_VIDEO_BUFFER_SIZE - 1, 0), 1);
	ASSERT_EQ(CalculateFECShardPackets(1 * ALVR_MAX_VIDEO_BUFFER_SIZE, 0), 1);
	ASSERT_EQ(CalculateFECShardPackets(253 * ALVR_MAX_VIDEO_BUFFER_SIZE - 1, 0), 1);
	ASSERT_EQ(CalculateFECShardPackets(253 * ALVR_MAX_VIDEO_BUFFER_SIZE, 0), 1);
	ASSERT_EQ(CalculateFECShardPackets(253 * ALVR_MAX_VIDEO_BUFFER_SIZE + 1, 0), 2);
	ASSERT_EQ(CalculateFECShardPackets(255 * ALVR_MAX_VIDEO_BUFFER_SIZE, 0), 2);
	ASSERT_EQ(CalculateFECShardPackets(256 * ALVR_MAX_VIDEO_BUFFER_SIZE, 0), 2);
	ASSERT_EQ(CalculateFECShardPackets(506 * ALVR_MAX_VIDEO_BUFFER_SIZE, 0), 2);
	ASSERT_EQ(CalculateFECShardPackets(506 * ALVR_MAX_VIDEO_BUFFER_SIZE + 1, 0), 3);


	ASSERT_EQ(CalculateFECShardPackets(250 * ALVR_MAX_VIDEO_BUFFER_SIZE, 1), 1);
	ASSERT_EQ(CalculateFECShardPackets(250 * ALVR_MAX_VIDEO_BUFFER_SIZE + 1, 1), 1);
	ASSERT_EQ(CalculateFECShardPackets(251 * ALVR_MAX_VIDEO_BUFFER_SIZE, 1), 1);
	ASSERT_EQ(CalculateFECShardPackets(251 * ALVR_MAX_VIDEO_BUFFER_SIZE + 1, 1), 2);
	ASSERT_EQ(CalculateFECShardPackets(253 * ALVR_MAX_VIDEO_BUFFER_SIZE, 1), 2);
}