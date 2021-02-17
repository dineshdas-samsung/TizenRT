/****************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * examples/hello/hello_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

/****************************************************************************
 * Pre Processor Definitions
 ****************************************************************************/

#define BUFFER_SIZE_S			1800
#define SAMPLE_FILE 			"/mnt/sample_file"
#define SINGLE_TEST_FILE_DIR		"/mnt/single"
#define SINGLE_TEST_FILE		"/mnt/single/test_file"
#define MULTI_TEST_FILE_DIR 		"/mnt/multiple"
#define MULTI_TEST_FILE 		"/mnt/multiple/test_file_"
#define TEST_FLAG_SINGLE_WRITE_APP 	1
#define TEST_FLAG_SINGLE_WRITE_OVE 	2
#define TEST_FLAG_SINGLE_READ_SEQ 	3
#define TEST_FLAG_SINGLE_READ_OVE 	4
#define TEST_FLAG_MULTIPLE_WRITE_APP 	5
#define TEST_FLAG_MULTIPLE_WRITE_OVE 	6
#define TEST_FLAG_MULTIPLE_READ_SEQ 	7
#define TEST_FLAG_MULTIPLE_READ_OVE 	8
#define TEST_FLAG_SINGLE_WRITE_MIX 	9
#define N_MULTIPLE_FILES 		12
#define TEST_FILE_NAME_LEN_MAX		27
#define TEST_ITR_MAX 			10000


/****************************************************************************
 * Public Variables
 ****************************************************************************/

double time_taken_append;
double time_taken_without_append;
int fd_multiple[N_MULTIPLE_FILES];
char rw_buf_s[BUFFER_SIZE_S + 1];
char buffer[N_MULTIPLE_FILES][TEST_FILE_NAME_LEN_MAX];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

int init_sample(void)
{
	int fd;
	int ret;
	int i;
	char sample_buf[] = "Writing data to the sample file\n";

	fd = open(SAMPLE_FILE, O_WRONLY | O_CREAT);
	if (fd < 0) {
		printf("Unable to open sample file: %s, fd = %d\n", SAMPLE_FILE, fd);
		return -ENOMEM;
	}

	for (i = 0; i < 200; i++) {
		ret = write(fd, sample_buf, 32);
		if (ret != 32) {
			printf("Unable to write to sample file, fd = %d\n", ret);
			return -1;
		}
	}

	close(fd);
	return OK;
}

void output(int flag, unsigned long time, int itr)
{
	printf("====================================================================================================\n");
	switch (flag) {
	case TEST_FLAG_SINGLE_WRITE_APP: 
		printf("Time taken for writing %d times to a single file consecutively is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
		break;
	
	case TEST_FLAG_SINGLE_WRITE_OVE:
		printf("Time taken for overwriting %d times to a single file at the start is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
		break;
	
	case TEST_FLAG_MULTIPLE_WRITE_APP:
		printf("Time taken for writing %d times consecutively to multiple files is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
		break;

	case TEST_FLAG_MULTIPLE_WRITE_OVE:
		printf("Time taken for writing %d times to multiple files at their start is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	case TEST_FLAG_SINGLE_WRITE_MIX:
                printf("Time taken for open-(write*8)-close-delete %d times to a single file is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	case TEST_FLAG_SINGLE_READ_SEQ:
                printf("Time taken for reading %d times from a single file consecutively is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	case TEST_FLAG_SINGLE_READ_OVE:
                printf("Time taken for reading %d times from a single file from the start is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;
	
	case TEST_FLAG_MULTIPLE_READ_SEQ:
                printf("Time taken for reading %d times consecutively from multiple files is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;
	
	case TEST_FLAG_MULTIPLE_READ_OVE:
                printf("Time taken for reading %d times from multiple files from the start is %lu.%06lu s\n", itr, (time)/1000000, (time)%1000000);
                break;

	default:
		break;
	}
	printf("====================================================================================================\n\n");
	return;
}

static int do_test(int fd, int opn)
{
	int ret = OK;
	int i;
	int j;
	struct timeval wr_time_start;
	struct timeval wr_time_stop;
	unsigned long tt;

	switch (opn) {
	case TEST_FLAG_SINGLE_WRITE_APP:
		gettimeofday(&wr_time_start, NULL);
		for (i = 0; i < 450; i++) {
			ret = write(fd, rw_buf_s, BUFFER_SIZE_S);
			if (ret != BUFFER_SIZE_S) { 
				printf("Unable to write to test file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
				return -1;
			}
		}
		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec)*1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		output(TEST_FLAG_SINGLE_WRITE_APP, tt, 450);

		ret = lseek(fd, 0, SEEK_SET);
		if (ret < 0) {
			printf("Unable to move file pointer to start\n");
			return -1;
		}

		gettimeofday(&wr_time_start, NULL);
                for (i = 0; i < 450; i++) {
                        ret = read(fd, rw_buf_s, BUFFER_SIZE_S);
                        if (ret != BUFFER_SIZE_S) {
                               printf("Unable to read to from file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
                                return -1;
                        }
                }
                gettimeofday(&wr_time_stop, NULL);
                tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec)*1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
                output(TEST_FLAG_SINGLE_READ_SEQ, tt, 450);
		break;

	case TEST_FLAG_SINGLE_WRITE_OVE:
		gettimeofday(&wr_time_start, NULL);
        	for(i = 0; i < 450; i++) {
                	fd = open(SINGLE_TEST_FILE, O_RDWR | O_CREAT | O_TRUNC);
                	if (fd < 0) {
                        	printf("Unable to do test perfomance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
                        	return -1;
                	}

                	ret = write(fd, rw_buf_s, BUFFER_SIZE_S);
                	if (ret != BUFFER_SIZE_S) {
                        	printf("Unable to do test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
                        	return -1;
                	}
                	close(fd);
        	}
        	gettimeofday(&wr_time_stop, NULL);
        	tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec)*1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
        	output(TEST_FLAG_SINGLE_WRITE_OVE, tt, 450);
		
		gettimeofday(&wr_time_start, NULL);
                for(i = 0; i < 450; i++) {
                        fd = open(SINGLE_TEST_FILE, O_RDONLY);
                        if(fd < 0) {
                                printf("Unable to do test perfomance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
                                return -1;
                        }

                        ret = read(fd, rw_buf_s, BUFFER_SIZE_S);
                        if (ret != BUFFER_SIZE_S) {
                                printf("Unable to do read test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
                                return -1;
                        }
                        close(fd);
                }
                gettimeofday(&wr_time_stop, NULL);
                tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec)*1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
                output(TEST_FLAG_SINGLE_READ_OVE, tt, 450);

		ret = unlink(SINGLE_TEST_FILE);
		if (ret < 0) {
			printf("Failed to remove file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
			return -1;
		}
		break;

	case TEST_FLAG_MULTIPLE_WRITE_APP:
		gettimeofday(&wr_time_start, NULL);
		for(i = 0; i < 37; i++) {
			for(j = 0; j < 12; j++) {
				ret = write(fd_multiple[j], rw_buf_s, BUFFER_SIZE_S);
                        	if (ret < 0) {
                                	printf("Unable to write to test file: %s, ret = ", buffer[j], ret);
                                	return -1;
                       		}
			}
		}
		gettimeofday(&wr_time_stop, NULL);
		tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec)*1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
		output(TEST_FLAG_MULTIPLE_WRITE_APP, tt, 37);

		for (i = 0; i < 12; i++) {
			ret = lseek(fd_multiple[i], 0, SEEK_SET);
			if (ret < 0) {
				printf("Unable to move file pointer for file %d to start, ret = %d, errno = %d\n", i+1, ret, errno);
				return -1;
			}
		}

		gettimeofday(&wr_time_start, NULL);
		for(i = 0; i < 37; i++) {
                        for(j = 0; j < 12; j++) {
                                ret = read(fd_multiple[j], rw_buf_s, BUFFER_SIZE_S);
                                if (ret != BUFFER_SIZE_S) {
                                        printf("Unable to read from test file: %s, ret = ", buffer[j], ret);
                                        return -1;
                                }
                        }
                }
                gettimeofday(&wr_time_stop, NULL);
                tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec)*1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
                output(TEST_FLAG_MULTIPLE_READ_SEQ, tt, 37);
		break;

	case TEST_FLAG_MULTIPLE_WRITE_OVE:
                gettimeofday(&wr_time_start, NULL);
                for (i = 0; i < 37; i++) {
                	for (j = 0; j < 12; j++) {
                		fd_multiple[j] = open(buffer[j], O_RDWR | O_CREAT | O_TRUNC);
                		if (fd_multiple[j] < 0) {
                        		printf("Unable to open file = %s for testing, fd = %d\n", buffer[j], fd_multiple[j]);
                        		return -1;
                		}
                		ret = write(fd_multiple[j], rw_buf_s, BUFFER_SIZE_S);
	                        if (ret < 0) {
	                                printf("Unable to do test performance on file: %s, ret = %d\n", MULTI_TEST_FILE, ret);
	                                return -1;
	                        }
	                        close(fd_multiple[j]);   
			}              
	         }
                gettimeofday(&wr_time_stop, NULL);
                tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec)*1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
                output(TEST_FLAG_MULTIPLE_WRITE_OVE, tt, 37);
                
		gettimeofday(&wr_time_start, NULL);
                for (i = 0; i < 37; i++) {
                        for (j = 0; j < 12; j++) {
                                fd_multiple[j] = open(buffer[j], O_RDONLY);
                                if (fd_multiple[j] < 0) {
                                        printf("Unable to open file = %s for testing, fd = %d\n", buffer[j], fd_multiple[j]);
                                        return -1;
                                }
                                ret = read(fd_multiple[j], rw_buf_s, BUFFER_SIZE_S);
                                if (ret != BUFFER_SIZE_S) {
                                        printf("Unable to do read test performance on file: %s, ret = %d\n", MULTI_TEST_FILE, ret);
                                        return -1;
                                }
                                close(fd_multiple[j]);
                        }
                }
                gettimeofday(&wr_time_stop, NULL);
                tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec)*1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
                output(TEST_FLAG_MULTIPLE_READ_OVE, tt, 37);

		for (i = 0; i < 12; i++) {
			ret = unlink(buffer[i]);
			if (ret < 0) {
				printf("Unable to delet file: %s, ret = %d\n", buffer[i], ret);
				return -1;
			}
		}
		break;

	case TEST_FLAG_SINGLE_WRITE_MIX:
                gettimeofday(&wr_time_start, NULL);
                for(i = 0; i < 56; i++) {
                        fd = open(SINGLE_TEST_FILE, O_WRONLY | O_CREAT);
                        if(fd < 0) {
                                printf("Unable to do test perfomance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
                                return -1;
                        }
			
			for (j = 0; j < 8; j++) {				
                        	ret = write(fd, rw_buf_s, BUFFER_SIZE_S);
				if (ret != BUFFER_SIZE_S) {
                         		printf("Unable to do test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
                                	return -1;
                        	}
			}

                        close(fd);
			unlink(SINGLE_TEST_FILE);
                }
                gettimeofday(&wr_time_stop, NULL);
                tt = (wr_time_stop.tv_sec - wr_time_start.tv_sec)*1000000 + (wr_time_stop.tv_usec - wr_time_start.tv_usec);
                output(TEST_FLAG_SINGLE_WRITE_MIX, tt, 56);
                break;

	default:
		break;
	}
	return ret;
}

static int init(void)
{
	int ret;
	int fd_sample = 0;
	int i;

	memset(rw_buf_s, 0, BUFFER_SIZE_S + 1);

        //Initializing sample file
        ret = init_sample();
        if(ret < 0) {
                printf("Unable to initalize the sample file: %s, ret = %d\n", SAMPLE_FILE, ret);
                return ret;
        }

	ret = mkdir(SINGLE_TEST_FILE_DIR, 0777);
        if (ret < 0) {
                printf("Unable to create directory for single file testing, ret = %d\n", ret);
                return -1;
        }

        ret = mkdir(MULTI_TEST_FILE_DIR, 0777);
        if (ret < 0) {
                printf("Unable to create directory for multiple file testing, ret = %d\n", ret);
                return -1;
        }

        //Populating buffers of two sizes for testing
        fd_sample = open(SAMPLE_FILE, O_RDONLY);
        if (fd_sample < 0) {
                printf("Unable to open sample file for reading, ret = %d\n", ret);
                return ret;
        }

        ret = read(fd_sample, rw_buf_s, BUFFER_SIZE_S);
        if (ret != BUFFER_SIZE_S) {
                printf("Unable to read from sample file, ret = %d\n", ret);
                return ret;
        }
	close(fd_sample);

	for (i = 0; i < 12; i++) {
		snprintf(buffer[i], 27, "%s%d", MULTI_TEST_FILE, i);
	}

	return OK;
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int fs_performance_main(int argc, char *argv[])
#endif
{
	if (argc < 2) {
		printf("Invalid input, argc = %d\n", argc);
		goto end;
	}

	int option = atoi(argv[1]);	//Which test case to be executed
	int ret = 0;
	int fd_single = 0;		//File Descriptor for single file in test environment
	int i;

	printf("\nChosen test option = %d\n\n", option);

	printf("Initializing test files\n");
	ret = init();
	if (ret < 0) {
		printf("Failed to initialize test case parameters, ret = %d\n", ret);
		goto errout;
	}
	printf("Initialization complete\n\n");

	switch (option) {
	case 1: goto SINGLE_APP;

	case 2: goto SINGLE_OVE;

	case 3: goto MULTIPLE_APP;

	case 4: goto MULTIPLE_OVE;

	case 5: goto SINGLE_MIX;

	default: break;
	}

SINGLE_APP:	
	//Do test on single file for continuous writing
	fd_single = open(SINGLE_TEST_FILE, O_RDWR | O_CREAT);
	if (fd_single < 0) {
		printf("Unable to open file for testing, fd = %d\n", fd_single);
		return -1;
	}

	ret = do_test(fd_single, TEST_FLAG_SINGLE_WRITE_APP);
	if (ret < 0) {
		printf("Unable to test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
		return -1;
	}

	close(fd_single);
	unlink(SINGLE_TEST_FILE);
	//goto end;

SINGLE_OVE:
	//Do test on single file for overwriting
	ret = do_test(fd_single, TEST_FLAG_SINGLE_WRITE_OVE);
	if (ret < 0) {
                printf("Unable to test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
                return -1;
        }
	//goto end;

MULTIPLE_APP:
	//Initilaize set of files for multiple file testing
	for (i = 0; i < 12; i++) {
        	fd_multiple[i] = open(buffer[i], O_RDWR | O_CREAT);
        	if (fd_multiple[i] < 0) {
                	printf("Unable to open file = !!!%s!!! for testing, fd = !!!%d!!!, errno = %d\n", buffer[i], fd_multiple[i], errno);
                	ret = fd_multiple[i];
			goto errout;
        	}
		close(fd_multiple[i]);
	}


	for (i = 0; i < 12; i++) {
                fd_multiple[i] = open(buffer[i], O_RDWR);
		if (fd_multiple[i] < 0) {
			printf("Unable to retrieve fd for %dth file, fd = %d, errno = %d\n", (i + 1), fd_multiple[i], errno);
			goto errout;
		}
        }

	printf("Multiple file initialization complete\n");
	sleep(3);

        ret = do_test(0, TEST_FLAG_MULTIPLE_WRITE_APP);
        if (ret < 0) {
                printf("Unable to test performance on multiple files, ret = %d\n", ret);
                return -1;
        }
	
	for (i = 0; i < 12; i++) {
                close(fd_multiple[i]);
		ret = unlink(buffer[i]);
		if (ret < 0) {
			printf("Unable to remove file: %s, ret = %d\n", buffer[i], ret);
			goto end;
		}
	}
	//goto end;

MULTIPLE_OVE:
	//Do test on mutliple file for overwriting
        ret = do_test(0, TEST_FLAG_MULTIPLE_WRITE_OVE);
        if (ret < 0) {
                printf("Unable to test performance on file: %s, ret = %d\n", MULTI_TEST_FILE, ret);
                return -1;
        }
	//goto end;

SINGLE_MIX:
	//Do test on single file for writing 7-8 times and delete it. Repeat k iteration
	ret = do_test(0, TEST_FLAG_SINGLE_WRITE_MIX);
	if (ret < 0) {
		printf("Unable to test performance on file: %s, ret = %d\n", SINGLE_TEST_FILE, ret);
		return -1;
	}

end:
	ret = OK;
errout:
	printf("Filesystem Performance Test Example Exits\n");
	return ret;
}

/* 
 * All tests should be conducted on an erased partitions (menu option)
 * Analyze gradual increase in write times as partition gets full
 * All tests with small and large buffers AND single and multiple files
 */
