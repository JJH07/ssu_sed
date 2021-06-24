#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>

#define BIT_b 1
#define BIT_u 2
#define BIT_us 4
#define BIT_i 8
#define BIT_is 16
#define BIT_e 32
#define BIT_es 64
#define BIT_d 128
#define BIT_p 256
#define BIT_s 512
#define BIT_P 1024
#define BIT_Ps 2048

int ssu_sed(int argc, char (*argv)[32]);
int open_file(const char *filename, const char *SRC, const char *DST);
int open_dir(char *path, const char *Target, const char *SRC, const char *DST, int depth);
bool setting_option(const char *opc, char *opd);
int _strlen(const char *str);
bool _strcmp(const char *x, const char *y);
int _strsearch(const char *str, const char *key, bool opt_b, bool opt_s);
void _strcat(char *DST, const char *SRC);
void Print_opt();

typedef struct{
	char *USERNAME;
	char *iSTRING;
	char *eSTRING;
	char *PATHNAME;
	int DEPTH;
	int bitset;
}option;

option option_table;

int main()
{
	char buf[128] = {0};	
	struct timeval stime, etime;	// 시간 측정 위한 변수입니다.
	
	while(1) // ctrl+c 입력하기 전까지 실행하게합니다.
	{
		char argv[10][32] = {0}; // 인자 최대 10개를 나눠 배열에 저장합니다.
		int argc = 0; // 인자 개수를 저장할 변수입니다
		bool FLAG_ARGC = false;
		printf("20122483 $ ");
		fgets(buf, sizeof(buf), stdin); // 명령어를 입력받아 buf에 저장합니다.
		buf[strlen(buf) - 1]= '\0'; // 개행문자를 삭제합니다.

		char *Token = strtok(buf, " ");
		while(Token)
		{
			if(argc == 10)
			{
				printf("ERR : arguments must be <= 10\n");
				FLAG_ARGC = true;
				break;
			}
			strcpy(argv[argc], Token);
			while(argv[argc][strlen(argv[argc]) - 1] == '\\')
			{
				argv[argc][strlen(argv[argc]) - 1] = ' ';
				Token = strtok(NULL, " ");
				if(Token)
				{
					strcat(argv[argc], Token);
				}
			}
			argc++;
			Token = strtok(NULL, " ");
		}
		if(!FLAG_ARGC)
		{
			gettimeofday(&stime, NULL);
			if(strcmp(argv[0], "ssu_sed"))
			{
				system(buf);
			}
			else
			{
				memset((option *)&option_table, 0, sizeof(option_table));
				option_table.DEPTH = 128;
				ssu_sed(argc, argv);
			}
			gettimeofday(&etime, NULL);
			printf("\ntime = %ld.%03ld\n", etime.tv_sec - stime.tv_sec, 1L * (etime.tv_usec - stime.tv_usec) / 1000);
		}
	}
	return 0;
}
int ssu_sed(int argc, char (*argv)[32]) // ssu_sed 함수입니다. 위의 argc와 argv를 인자로 받습니다.
{
	char path[1024] = {0}; // 경로를 저장할 배열입니다.
	char *Target, *SRC_STR, *DEST_STR;
	struct stat statbuf; // stat구조체입니다.
	
	if(argc < 4)
	{
		puts("ssu_sed() ERROR : must have 3 arguments [Target, SRC_STR, DEST_STR]");
		return -1;
	}
	Target = argv[1];
	SRC_STR = argv[2];
	DEST_STR = argv[3];
	
	for(int i = 4; i < argc; i++)
	{
		char *opd = (i + 1 < argc ? argv[i + 1] : NULL);
		if(!setting_option(argv[i], opd))
		{
			puts("ssu_sed() ERROR : invalid arguments");
			puts("===========================================\n#\tMANUAL\n#\n#\tssu_sed [TARGET] [SRC_STR] [DEST_STR] [OPTION]\n#\n#\t[TARGET] = FILE or DIRECTORY\n#\n#\t[SRC_STR] = SEARCH STRING\n#\n#\t[DEST_STR] = SUBSTITUE STRING\n#\n#\t[OPTION] = -b\n#\t         | -u [USERNAME]\n#\t         | -i [STRING]\n#\t         | -e [STRING]\n#\t         | -d [DEPTH]\n#\t         | -p\n#\t         | -s\n#\t         | -P [PATHNAME]");
			return -1;
		}
		if(argv[i][1] == 'u' || argv[i][1] == 'i' || argv[i][1] == 'e' || argv[i][1] == 'd' || argv[i][1] == 'P')
		{
			++i;
		}
	}

	for(int i = 0; i < argc; i++)
	{
		printf("arg%d : %s\n", i, argv[i]);
	}

	if(lstat(Target, &statbuf) < 0)
	{
		puts("ssu_sed() ERROR : Target file not exist");  // 파일로부터 stat 구조체를 얻습니다.
		return -1;
	}
	
	if(option_table.USERNAME != NULL) // -u option
	{
		struct passwd *user;
		if((user = getpwnam(option_table.USERNAME)) == NULL)
		{
			return -1;
		}
		if(statbuf.st_uid != user->pw_uid)
		{
			return -1;
		}
	}
	if(S_ISDIR(statbuf.st_mode))
	{
		chdir(Target);
		getcwd(path, 1024);
		open_dir(path, Target, SRC_STR, DEST_STR, 0);
		chdir("../");
	}
	else if(S_ISREG(statbuf.st_mode))
	{
		getcwd(path, 1024);
		if(open_file(Target, SRC_STR, DEST_STR) == -1)
		{
			_strcat(path, " : failed");
		}
		else
		{
			_strcat(path, " : success");
		}
		puts(path);
	}
	return 0;
}

int open_dir(char *path, const char *Target, const char *SRC, const char *DST, int depth) // 이름이 디렉토리였을 때 실행되는 함수입니다.
{
	DIR *dp; //디렉토리 포인터입니다.
	FILE *fp; //파일포인터입니다.
	int fd; // 파일디스크립터 변수입니다.
	struct stat statbuf; // stat구조체변수입니다.
	struct dirent *dirp; // dirent 구조체 변수입니다.
	char nPath[1024]; // 파일 경로가 들어갈 변수입니다.
	
	if((dp = opendir(path)) == NULL) // 디렉토리를 엽니다.
	{
		puts("open_dr() ERROR : fail to call opendir()");
		return -1;
	}
	while((dirp = readdir(dp)) != NULL)
	{
		if(_strcmp(dirp->d_name, "..") || _strcmp(dirp->d_name, "."))
		{
			continue;
		}
		if(lstat(dirp->d_name, &statbuf) < 0)
		{
			puts("open_dir() ERROR : fail to call lstat()");
			return -1;
		}
		if(S_ISDIR(statbuf.st_mode) && (depth + 1 <= option_table.DEPTH))
		{
			chdir(dirp->d_name);
			getcwd(nPath, 1024);
			open_dir(nPath, Target, SRC, DST, depth + 1);
			chdir("../");
		}
		else if(S_ISREG(statbuf.st_mode))
		{
			_strcat(path, dirp->d_name);
			if(open_file(dirp->d_name, SRC, DST) == -1)
			{
				_strcat(path, " : failed");
			}
			else
			{
				_strcat(path, " : success");
			}
			puts(path);
		}
	}
	return 0;
}

int open_file(const char *filename, const char *SRC, const char *DST)
{
	FILE *fp; // 파일포인터를 저장할 포인터변수입니다.
	int fd; // 파일디스크립터를 저장할 변수입니다.
	char text[256][256], nText[256][256];
	int line_cnt = 0, line_fixed[256];
	if((fp = fopen(filename, "r+")) == NULL)
	{
		puts("fopen error");
		return -1;
	}
	while((fgets(text[line_cnt], 256, fp)) != NULL)
	{
		int _len = _strlen(text[line_cnt]);
		line_cnt++;
	}
	fclose(fp);
	if(option_table.iSTRING != NULL)
	{
		bool included = false;
		for(int line = 0; line < line_cnt && !included; line++)
		{
			int _len = _strlen(text[line]);
			for(int i = 0; i < _len && !included; i++)
			{
				if(_strsearch(text[line] + i, option_table.iSTRING, option_table.bitset & BIT_b, option_table.bitset & (BIT_s + BIT_is)) != -1)
				{
					included = true;
				}
			}
		}
		if(!included)
		{
			return -1;
		}
	}
	if(option_table.eSTRING != NULL)
	{
		for(int line = 0; line < line_cnt; line++)
		{
			int _len = _strlen(text[line]);
			for(int i = 0; i < _len; i++)
			{
				if(_strsearch(text[line] + i, option_table.eSTRING, option_table.bitset & BIT_b, option_table.bitset & (BIT_s + BIT_es)) != -1)
				{
					return -1;
				}
			}
		}
	}
	char fname[256];
	fname[0] = '\0';
	if(option_table.bitset & BIT_P)
	{
		_strcat(fname, option_table.PATHNAME);
		int _len = _strlen(fname);
		if(fname[_len - 1] != '/')
		{
			_strcat(fname, "/");
		}
	}
	_strcat(fname, filename);

	if((fd = open(fname, O_RDWR|O_TRUNC|O_CREAT, 0751)) == -1)
	{
		puts("fopen error");
	}
	for(int line = 0; line < line_cnt; line++)
	{
		char buf[256];
		int _len = _strlen(text[line]), buf_size = 0;
		line_fixed[line] = 0;
		for(int i = 0; i < _len; i++)
		{
			int jump = _strsearch(text[line] + i, SRC, option_table.bitset & BIT_b, option_table.bitset & BIT_s);
			if(jump == -1)
			{
				buf[buf_size++] = text[line][i];
			}
			else
			{
				line_fixed[line] = 1;
				for(int sidx = 0; sidx < _strlen(DST); sidx++)
				{
					buf[buf_size++] = DST[sidx];
				}
				i += jump - 1;
			}
		}
		write(fd, buf, buf_size);
	}
	close(fd);

	if(option_table.bitset & BIT_p)
	{
		char path[1024];
		for(int line = 0; line < line_cnt; line++)
		{
			int number = line + 1, idx = 0;
			char li[256];
			while(number > 0)
			{
				li[idx++] = (number % 10) + '0';
				number /= 10;
			}
			if(line_fixed[line])
			{
				getcwd(path, 1024);
				_strcat(path, " : ");

				char t[256];
				for(int i = 0; i < idx; i++)
				{
					t[i] = li[idx - 1 - i];
				}
				t[idx] = '\0';
				_strcat(path, t);
				
				puts(path);
			}
		}
	}

	return 0;
}
bool setting_option(const char *opt, char *s)
{
	int len = _strlen(opt), bit = -1;
	if(opt[0] != '-' || len > 3)
	{
		return false;
	}
	if(len == 3 && opt[2] != 's')
	{
		return false;
	}
	switch(opt[1])
	{
		case 'b':
		{
			bit = BIT_b;
			break;
		}
		case 'u':
		{
			bit = BIT_u;
			break;
		}
		case 'i':
		{
			bit = BIT_i;
			break;
		}
		case 'e':
		{
			bit = BIT_e;
			break;
		}
		case 'd':
		{
			bit = BIT_d;
			break;
		}
		case 'p':
		{
			bit = BIT_p;
			break;
		}
		case 's':
		{
			bit = BIT_s;
			break;
		}
		case 'P':
		{
			bit = BIT_P;
			break;
		}
		default:
		{
			return false;
		}
	}
	if(opt[2] == 's')
	{
		if(bit == BIT_u || bit == BIT_i || bit == BIT_e || bit == BIT_P)
		{
			bit *= 2;
		}
		else
		{
			return false;
		}
	}
	if(option_table.bitset & bit)
	{
		return false;
	}

	if(bit == BIT_u || bit == BIT_us)
	{
		if(s == NULL)
		{
			return false;
		}
		option_table.USERNAME = s;
	}
	else if(bit == BIT_i || bit == BIT_is)
	{
		if(s == NULL)
		{
			return false;
		}
		option_table.iSTRING = s;
	}
	else if(bit == BIT_e || bit == BIT_es)
	{
		if(s == NULL)
		{
			return false;
		}
		option_table.eSTRING = s;
	}
	else if(bit == BIT_P || bit == BIT_Ps)
	{
		if(s == NULL)
		{
			return false;
		}
		option_table.PATHNAME = s;
	}
	else if(bit == BIT_d)
	{
		int number = 0;
		if(s == NULL)
		{
			return false;
		}
		for(int i = 0; i < _strlen(s); i++)
		{
			if(s[i] < '0' || s[i] > '9')
			{
				return false;
			}
			number = number * 10 + (s[i] - '0');
		}
		option_table.DEPTH = number;
	}
	option_table.bitset += bit;
	return true;
}
int _strlen(const char *str)
{
	int ret = 0;
	while(*str != '\0')
	{
		++ret;
		++str;
	}
	return ret;
}
bool _strcmp(const char *x, const char *y)
{
	int l_x = _strlen(x);
	int l_y = _strlen(y);
	if(l_x != l_y)
	{
		return false;
	}
	for(int i = 0; i < l_x; i++)
	{
		if(x[i] != y[i])
		{
			return false;
		}
	}
	return true;
}
int _strsearch(const char *str, const char *key, bool opt_b, bool opt_s)
{
	int _str_len = _strlen(str);
	int _key_len = _strlen(key);
	if(_str_len < _key_len)
	{
		return -1;
	}
	int _str_idx, _key_idx;
	for(_str_idx = 0, _key_idx = 0; _str_idx < _str_len && _key_idx < _key_len; _str_idx++)
	{
		char word = str[_str_idx];
		char k = key[_key_idx++];
		if(opt_b && word == ' ')
		{
			continue;
		}
		if(opt_s)
		{
			if(word >= 'A' && word <= 'Z')
			{
				word = 'a' + (word - 'A');
			}
			if(k >= 'A' && k <= 'Z')
			{
				k = 'a' + (k - 'A');
			}
		}
		if(word != k)
		{
			return -1;
		}
	}
	return _key_idx != _key_len ? -1 : _str_idx;
}
void _strcat(char *DST, const char *SRC)
{
	int D_len = _strlen(DST);
	int S_len = _strlen(SRC);
	for(int i = D_len; i <= D_len + S_len; i++)
	{
		DST[i] = SRC[i - D_len];
	}
}
void Print_opt()
{
	printf("USERNAME : %s\n", option_table.USERNAME);
	printf("iSTRING : %s\n", option_table.iSTRING);
	printf("eSTRING : %s\n", option_table.eSTRING);
	printf("PATHNAME : %s\n", option_table.PATHNAME);
	printf("DEPTH : %d\n", option_table.DEPTH);
	printf("b = %d\n", option_table.bitset & BIT_b);
	printf("u = %d\n", option_table.bitset & BIT_u);
	printf("us = %d\n", option_table.bitset & BIT_us);
	printf("i = %d\n", option_table.bitset & BIT_i);
	printf("is = %d\n", option_table.bitset & BIT_is);
	printf("e = %d\n", option_table.bitset & BIT_e);
	printf("es = %d\n", option_table.bitset & BIT_es);
	printf("d = %d\n", option_table.bitset & BIT_d);
	printf("p = %d\n", option_table.bitset & BIT_p);
	printf("s = %d\n", option_table.bitset & BIT_s);
	printf("P = %d\n", option_table.bitset & BIT_P);
	printf("Ps = %d\n", option_table.bitset & BIT_Ps);
}