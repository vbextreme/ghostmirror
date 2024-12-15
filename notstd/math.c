#include <notstd/mth.h>
#include <notstd/mathmacro.h>
#include <notstd/memory.h>
#include <notstd/debug.h>
#include <string.h>

#include <math.h>
#include <time.h>
#include <stdlib.h>

double mth_gtor(double gradi){
	return ((gradi*PI2)/ANGLE_FULL);
}

void mth_random_begin(void){
	srand((unsigned)time(NULL));
}

int mth_random(int n){
	return rand() % n;
}

int mth_random_range(int min,int max){
	return min + (rand() % (max-min+1));
}

float mth_random_gauss(float media, float stddeviation){
	float x1, x2, w, y1;
	static float y2;
	static int use_last = 0;
	
	if (use_last)
	{
		y1 = y2;
		use_last = 0;
	}
	else
	{
		do {
			x1 = 2.0 * mth_random_f01() - 1.0;
			x2 = 2.0 * mth_random_f01() - 1.0;
			w = x1 * x1 + x2 * x2;
		} while ( w >= 1.0 );

		w = sqrt( (-2.0 * log( w ) ) / w );
		y1 = x1 * w;
		y2 = x2 * w;
		use_last = 1;
	}

	return (media + y1 * stddeviation);
}

double mth_random_f01(void){
	return ((double)(rand()) / ((double)RAND_MAX + 1.0));
}

void mth_random_string_azAZ09(char* out, size_t size){
	static char const * const azAZ09 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVQXYZ0123456789";
	size--;
	while( size-->0 ){
		size_t r = mth_random(62);
		*out++ = azAZ09[r];
	}
	*out = 0;
}

rndUnique_s* mth_random_unique_ctor(rndUnique_s* ru, int min, int max){
	ru->min    = min;
	ru->max    = max;
	ru->count  = ru->max - ru->min;
	ru->buffer = MANY(int, ru->count);
	return ru;
}

rndUnique_s* mth_random_unique_dtor(rndUnique_s* ru){
	DELETE(ru->buffer);
	return ru;
}

rndUnique_s* mth_random_unique_reset(rndUnique_s* ru){
	for( unsigned i = 0; i < ru->count; ++i ){
		ru->buffer[i] = i + ru->min;
	}
	ru->it = ru->count;
	return ru;
}

int mth_random_unique(rndUnique_s* ru, int* extract){
	if( !ru->it ) return -1;
	const unsigned rp = mth_random(ru->it);
	--ru->it;
	int tmp = ru->buffer[ru->it];
	ru->buffer[ru->it] = ru->buffer[rp];
	ru->buffer[rp] = tmp;
	*extract = ru->buffer[ru->it];
	return 0;
}

void mth_rotate(float *x,float *y,float centerx,float centery,float rad){
	float crad=cos(rad);
	float srad=sin(rad);
	double nx= *x - centerx;
	double ny= *y - centery;
	*x=((nx * crad) - (ny * srad)) + centerx;
	*y=((nx * srad) + (ny * crad)) + centery;
}

unsigned long mth_date_julian_time(double jd){
	struct tm date = {0};
	
	long jdi, b;
	long c,d,e,g,g1;
	jd += 0.5;
	jdi = jd;
	if (jdi > 2299160) {
		long a = (jdi - 1867216.25)/36524.25;
		b = jdi + 1 + a - a/4;
	}
	else b = jdi;
	
	c = b + 1524;
	d = (c - 122.1)/365.25;
	e = 365.25 * d;
	g = (c - e)/30.6001;
	g1 = 30.6001 * g;
	date.tm_wday = c - e - g1;
	date.tm_hour = (jd - jdi) * 24.0;
	if (g <= 13) date.tm_mon = g - 1;
	else date.tm_mon = g - 13;
	if (date.tm_mon > 2) date.tm_year = (d - 4716)-1900;
	else date.tm_year = (d - 4715)-1900;
	return mktime(&date);	
}

double mth_date_julian(int year,int month,double day){
	int a,b=0,c,e;
	if (month < 3){
		year--;
		month += 12;
	}
	if (year > 1582 || (year == 1582 && month>10) || (year == 1582 && month==10 && day > 15)){
		a=year/100;
		b=2-a+a/4;
	}
	c = 365.25*year;
	e = 30.6001*(month+1);
	return b+c+e+day+1720994.5;
}

int mth_date_julian_ut(int d,int m,int y){
	int mm, yy;
	int k1, k2, k3;
	int j;
	yy = y - (int)((12 - m) / 10);
	mm = m + 9;
	if (mm >= 12){
		mm = mm - 12;
	}
	k1 = (int)(365.25 * (yy + 4712));
	k2 = (int)(30.6001 * mm + 0.5);
	k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
	// 'j' for dates in Julian calendar:
	j = k1 + k2 + d + 59;
	if (j > 2299160){
		// For Gregorian calendar:
		j = j - k3; // 'j' is the Julian date at 12h UT (Universal Time)
	}
	return j;
}


double mth_sun_position(double j){
	double n,x,e,l,dl,v;
	//double m2;
	int i;
	
	n=360/365.2422*j;
	i=n/360;
	n=n-i*360.0;
	x=n-3.762863;
	if (x<0) x += 360;
	x *= RAD;
	e=x;
	do {
	dl=e-.016718*sin(e)-x;
	e=e-dl/(1-.016718*cos(e));
	} while (fabs(dl)>=SMALL_FLOAT);
	v=360/PI*atan(1.01686011182*tan(e/2));
	l=v+282.596403;
	i=l/360;
	l=l-i*360.0;
	return l;
}

double mth_moon_position(double j, double ls){
	double ms,l,mm,ev,sms,ae,ec;
	int i;
	ms = 0.985647332099*j - 3.762863;
	if (ms < 0) ms += 360.0;
	l = 13.176396*j + 64.975464;
	i = l/360;
	l = l - i*360.0;
	if (l < 0) l += 360.0;
	mm = l-0.1114041*j-349.383063;
	i = mm/360;
	mm -= i*360.0;
	//n = 151.950429 - 0.0529539*j;
	//i = n/360;
	//n -= i*360.0;
	ev = 1.2739*sin((2*(l-ls)-mm)*RAD);
	sms = sin(ms*RAD);
	ae = 0.1858*sms;
	mm += ev-ae- 0.37*sms;
	ec = 6.2886*sin(mm*RAD);
	l += ev+ec-ae+ 0.214*sin(2*mm*RAD);
	l= 0.6583*sin(2*(l-ls)*RAD)+l;
	return l;
}

double mth_moon_phase(int year,int month,int day, double hour, int* ip){
	double j= mth_date_julian(year,month,(double)day+hour/24.0)-2444238.5;
	double ls = mth_sun_position(j);
	double lm = mth_moon_position(j, ls);
	double t = lm - ls;
	if (t < 0) t += 360;
	if( ip ) *ip = (int)((t + 22.5)/45) & 0x7;
	return (1.0 - cos((lm - ls)*RAD))/2;
}

void mth_mat_addi(int** r, int** a, int** b, size_t szr, size_t szc){
	for(size_t row=0; row < szr; row++){
		for( size_t col=0; col < szc ; col++){
			r[row][col] = a[row][col] + b[row][col];
		}
	}
}

void mth_mat_subi(int** r, int** a, int** b, size_t szr, size_t szc){
	for(size_t row=0; row < szr; row++){
		for( size_t col=0; col < szc ; col++){
			r[row][col] = a[row][col] - b[row][col];
		}
	}
}

void mth_mat_imuli(int** r, int a, int** b, size_t szr, size_t szc){
	for(size_t row=0; row < szr; row++){
		for( size_t col=0; col < szc ; col++){
			r[row][col] = a * b[row][col];
		}
	}
}

void mth_mat_muli(int** r, int** a, int** b, size_t szr, size_t szc){
	for(size_t row=0; row < szr; row++){
		for( size_t col=0; col < szc ; col++){
			r[row][col] = 0;
			for( size_t rc = 0; rc < szc; ++rc){
				r[row][col] += a[row][col] * b[row][col];
			}
		}
	}
}

int mth_mat_determinant2(int** a){
	return a[0][0] * a[1][1] - a[0][1] * a[1][0];
}

int mth_mat_determinant3(int** a){
	return a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) +
	       a[0][1] * (a[1][2] * a[2][0] - a[1][0] * a[2][2]) +
	       a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]) ;
}

void mth_fqr_generate(short int* buffer,int samplerate,double durata,double stfq,int loopfq,double fq,double amplitude,int fase){
	int szb = samplerate * durata;
	double byteforp = (double)samplerate / fq;
	double v;
	int i;
	int fqsample;
	
	if (loopfq == 0){
		fqsample = szb;
	}
	else{
		fqsample = ((double)szb / (durata / (1/fq))) * (double)loopfq;
	}

	if (fase != 0){
		fase = ((double)szb / (durata / (1/fq))) / fase;
	}

	for (i = samplerate * stfq; i < szb ; i++){
		fase++;

		v = PI2;
		v = amplitude * sin((double)fase * v / byteforp);

		if ( v > 32766)
            buffer[i] = 32766;
		else if (v < -32766)
            buffer[i] = -32766;
		else
			buffer[i] = (short int) v;
		if (fase == fqsample) break;
	}
}

__private void _fft(double complex buf[], double complex out[], int n, int step){
	if (step < n) {
		_fft(out, buf, n, step * 2);
		_fft(out + step, buf + step, n, step * 2);
		
		int i;
		for (i = 0; i < n; i += 2 * step) {
			double complex t = cexp(-I * PI * i / n) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + n)/2] = out[i] - t;
		}
	}
}

double complex* mth_fft(double complex buf[], int n){
	double complex* out = (double complex*) malloc(sizeof(double complex) * n);
	_fft(out, buf, n, 1);
	return out;
}

double mth_bbppigreco(long int i){
	double k;
	double sum = 0;
	if( i <= 0) { return -1.0; }
	for(k=0; k<i; ++k)
		sum = sum + ((4/(8*k+1))-(2/(8*k+4))-(1/(8*k+5))-(1/(8*k+6)))*(1/(pow(16,k)));
	return sum;
}

__const size_t mth_round_up_power_two(size_t n){
	if( n < 3 ) return 2;
	--n;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
	++n;
	return n;
}

#define KILO (1000UL)
size_t mth_si_prefix_translate_base(const sip_e sibase){
	static const size_t base[] = {
		[MTH_SIP_ONE]   = 1UL,
		[MTH_SIP_DECA]  = 10UL,
		[MTH_SIP_HECTO] = 100UL,
		[MTH_SIP_KILO]  = KILO,
		[MTH_SIP_MEGA]  = KILO*KILO,
		[MTH_SIP_GIGA]  = KILO*KILO*KILO,
		[MTH_SIP_TERA]  = KILO*KILO*KILO*KILO,
		[MTH_SIP_PETA]  = KILO*KILO*KILO*KILO*KILO,
		[MTH_SIP_EXA]   = KILO*KILO*KILO*KILO*KILO*KILO,
		[MTH_SIP_ZETTA] = KILO*KILO*KILO*KILO*KILO*KILO*KILO,
		[MTH_SIP_YOTTA] = KILO*KILO*KILO*KILO*KILO*KILO*KILO*KILO
	};
	return base[sibase];
}

const char* mth_si_prefix_translate_short_string(const sip_e sibase){
	static const char* base[] = {
		[MTH_SIP_ONE]   = "  ",
		[MTH_SIP_DECA]  = "da",
		[MTH_SIP_HECTO] = "h ",
		[MTH_SIP_KILO]  = "k ",
		[MTH_SIP_MEGA]  = "M ",
		[MTH_SIP_GIGA]  = "G ",
		[MTH_SIP_TERA]  = "T ",
		[MTH_SIP_PETA]  = "P ",
		[MTH_SIP_EXA]   = "E ",
		[MTH_SIP_ZETTA] = "Z ",
		[MTH_SIP_YOTTA] = "Y "
	};
	return base[sibase];
}

double mth_si_prefix_base(sip_e* siout, const double num){
	if( num <= 0 ){
		if( siout ) *siout = 0;	
		return num;
	}
	size_t si = MTH_SIP_YOTTA;
	double base;
	while( num < (base = mth_si_prefix_translate_base(si)) ){
		--si;
	}
	if( siout ) *siout = si;
	return num / base;
}

#define KiBYTE (1024UL)

size_t mth_iec_prefix_translate_base(const iecp_e b){
	static size_t base[] = {
		[MTH_IEC_BYTE]      = 1UL,
		[MTH_IEC_KILOBYTE]  = KiBYTE,
		[MTH_IEC_MEGABYTE]  = KiBYTE*KiBYTE,
		[MTH_IEC_GIGABYTE]  = KiBYTE*KiBYTE*KiBYTE,
		[MTH_IEC_TERABYTE]  = KiBYTE*KiBYTE*KiBYTE*KiBYTE,
		[MTH_IEC_PETABYTE]  = KiBYTE*KiBYTE*KiBYTE*KiBYTE*KiBYTE,
		[MTH_IEC_EXABYTE]   = KiBYTE*KiBYTE*KiBYTE*KiBYTE*KiBYTE*KiBYTE,
		[MTH_IEC_ZETTABYTE] = KiBYTE*KiBYTE*KiBYTE*KiBYTE*KiBYTE*KiBYTE*KiBYTE,
		[MTH_IEC_YOTTABYTE] = KiBYTE*KiBYTE*KiBYTE*KiBYTE*KiBYTE*KiBYTE*KiBYTE*KiBYTE
	};
	return base[b];
}

const char* mth_iec_prefix_translate_short_string(const iecp_e b){
	static const char* base[] = {
		[MTH_IEC_BYTE]      = "B  ",
		[MTH_IEC_KILOBYTE]  = "KiB",
		[MTH_IEC_MEGABYTE]  = "MiB",
		[MTH_IEC_GIGABYTE]  = "GiB",
		[MTH_IEC_TERABYTE]  = "TiB",
		[MTH_IEC_PETABYTE]  = "PiB",
		[MTH_IEC_EXABYTE]   = "EiB",
		[MTH_IEC_ZETTABYTE] = "ZiB",
		[MTH_IEC_YOTTABYTE] = "YiB"
	};
	return base[b];
}

double mth_ice_prefix_base(iecp_e* iecout, const double num){
	if( num <= 0 ){
		if( iecout ) *iecout = 0;	
		return num;
	}
	size_t iec = MTH_IEC_EXABYTE;
	double base;
	while( num < (base = mth_iec_prefix_translate_base(iec)) ){
		--iec;
	}
	if( iecout ) *iecout = iec;
	return num / base;
}

__private const unsigned char base64et[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
__private const unsigned char base64dt[80] = {
	['A' - '+'] = 0,  ['B' - '+'] = 1,  ['C' - '+'] = 2,  ['D' - '+'] = 3,	['E' - '+'] = 4,  ['F' - '+'] = 5,  ['G' - '+'] = 6,
	['H' - '+'] = 7,  ['I' - '+'] = 8,  ['J' - '+'] = 9,  ['K' - '+'] = 10, ['L' - '+'] = 11, ['M' - '+'] = 12, ['N' - '+'] = 13,
	['O' - '+'] = 14, ['P' - '+'] = 15, ['Q' - '+'] = 16, ['R' - '+'] = 17, ['S' - '+'] = 18, ['T' - '+'] = 19,	['U' - '+'] = 20, 
	['V' - '+'] = 21, ['W' - '+'] = 22, ['X' - '+'] = 23, ['Y' - '+'] = 24, ['Z' - '+'] = 25, 
	['a' - '+'] = 26, ['b' - '+'] = 27, ['c' - '+'] = 28, ['d' - '+'] = 29, ['e' - '+'] = 30, ['f' - '+'] = 31, ['g' - '+'] = 32, 
	['h' - '+'] = 33, ['i' - '+'] = 34, ['j' - '+'] = 35, ['k' - '+'] = 36, ['l' - '+'] = 37, ['m' - '+'] = 38, ['n' - '+'] = 39,
	['o' - '+'] = 40, ['p' - '+'] = 41, ['q' - '+'] = 42, ['r' - '+'] = 43,	['s' - '+'] = 44, ['t' - '+'] = 45,	['u' - '+'] = 46, 
	['v' - '+'] = 47, ['w' - '+'] = 48, ['x' - '+'] = 49, ['y' - '+'] = 50, ['z' - '+'] = 51, 
	['0' - '+'] = 52, ['1' - '+'] = 53, ['2' - '+'] = 54, ['3' - '+'] = 55, ['4' - '+'] = 56, ['5' - '+'] = 57, ['6' - '+'] = 58, 
	['7' - '+'] = 59, ['8' - '+'] = 60, ['9' - '+'] = 61,
	['+' - '+'] = 62, ['/' - '+'] = 63
};

char* base64_encode(const void* src, const size_t size){
	size_t una = size % 3;
	size_t ali = (size - una) / 3;
	una = 3 - una;
	const char* data = src;
	const size_t outsize = (size * 4) / 3 + 4;

	dbg_info("size:%lu outsize:%lu unaligned:%lu counter:%lu", size, outsize, una, ali);

	char* ret = MANY(char, outsize);
	char* next = ret;

	while( ali --> 0 ){
		*next++ = base64et[data[0] >> 2];
		*next++ = base64et[((data[0] & 0x03) << 4) | (data[1] >> 4)];
		*next++ = base64et[((data[1] & 0x0F) << 2) | (data[2] >> 6)];
		*next++ = base64et[data[2] & 0x3F];
		data += 3;
	}
	
	switch( una ){
		case 1:
			*next++ = base64et[data[0] >> 2];
			*next++ = base64et[((data[0] & 0x03) << 4) | (data[1] >> 4)];
			*next++ = base64et[((data[1] & 0x0F) << 2)];
			*next++ = '=';
		break;
	
		case 2:
			*next++ = base64et[data[0] >> 2];
			*next++ = base64et[((data[0] & 0x03) << 4)];
			*next++ = '=';
			*next++ = '=';
		break;
	}
	
	*next = 0;
	mem_header(ret)->len = next - ret;
	return ret;
}

void* base64_decode(size_t* size, const char* b64){
	const unsigned char* str = (const unsigned char*)b64;
	const size_t len = strlen(b64);
	size_t una = 0;
	if( str[len - 1] == '=' ) ++una;
	if( str[len - 2] == '=' ) ++una;
	
	const size_t countali = (len / 4) * 3 - una;
	if( size ) *size = countali;
	
	void* data = MANY(char, countali);
	char* next = data;
	
	size_t count = (len / 4) - 1;
	
	dbg_info("len:%lu count:%lu cali:%lu una:%lu", len, count, countali, una);
	
	while( count --> 0 ){
		*next++ = (base64dt[str[0] - '+'] << 2) | (base64dt[str[1] - '+'] >> 4);
		*next++ = (base64dt[str[1] - '+'] << 4) | (base64dt[str[2] - '+'] >> 2);
		*next++ = (base64dt[str[2] - '+'] << 6) |  base64dt[str[3] - '+'];
		str += 4;
	}
	
	switch( una ){
		case 0:
			*next++ = (base64dt[str[0] - '+'] << 2) | (base64dt[str[1] - '+'] >> 4);
			*next++ = (base64dt[str[1] - '+'] << 4) | (base64dt[str[2] - '+'] >> 2);
			*next++ = (base64dt[str[2] - '+'] << 6) |  base64dt[str[3] - '+'];
		break;
		case 1:
			*next++ = (base64dt[str[0] - '+'] << 2) | (base64dt[str[1] - '+'] >> 4);
			*next++ = (base64dt[str[1] - '+'] << 4) | (base64dt[str[2] - '+'] >> 2);
		break;
		case 2:
			*next++ = (base64dt[str[0] - '+'] << 2) | (base64dt[str[1] - '+'] >> 4);
		break;
	}
	
	mem_header(data)->len = countali;
	return data;
}

__private const uint16_t crc16table[256] ={
	0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
	0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
	0x0919, 0x1890, 0x2A0B, 0x3B82, 0x4F3D, 0x5EB4, 0x6C2F, 0x7DA6,
	0x8551, 0x94D8, 0xA643, 0xB7CA, 0xC375, 0xD2FC, 0xE067, 0xF1EE,
	0x1232, 0x03BB, 0x3120, 0x20A9, 0x5416, 0x459F, 0x7704, 0x668D,
	0x9E7A, 0x8FF3, 0xBD68, 0xACE1, 0xD85E, 0xC9D7, 0xFB4C, 0xEAC5,
	0x1B2B, 0x0AA2, 0x3839, 0x29B0, 0x5D0F, 0x4C86, 0x7E1D, 0x6F94,
	0x9763, 0x86EA, 0xB471, 0xA5F8, 0xD147, 0xC0CE, 0xF255, 0xE3DC,
	0x2464, 0x35ED, 0x0776, 0x16FF, 0x6240, 0x73C9, 0x4152, 0x50DB,
	0xA82C, 0xB9A5, 0x8B3E, 0x9AB7, 0xEE08, 0xFF81, 0xCD1A, 0xDC93,
	0x2D7D, 0x3CF4, 0x0E6F, 0x1FE6, 0x6B59, 0x7AD0, 0x484B, 0x59C2,
	0xA135, 0xB0BC, 0x8227, 0x93AE, 0xE711, 0xF698, 0xC403, 0xD58A,
	0x3656, 0x27DF, 0x1544, 0x04CD, 0x7072, 0x61FB, 0x5360, 0x42E9,
	0xBA1E, 0xAB97, 0x990C, 0x8885, 0xFC3A, 0xEDB3, 0xDF28, 0xCEA1,
	0x3F4F, 0x2EC6, 0x1C5D, 0x0DD4, 0x796B, 0x68E2, 0x5A79, 0x4BF0,
	0xB307, 0xA28E, 0x9015, 0x819C, 0xF523, 0xE4AA, 0xD631, 0xC7B8,
	0x48C8, 0x5941, 0x6BDA, 0x7A53, 0x0EEC, 0x1F65, 0x2DFE, 0x3C77,
	0xC480, 0xD509, 0xE792, 0xF61B, 0x82A4, 0x932D, 0xA1B6, 0xB03F,
	0x41D1, 0x5058, 0x62C3, 0x734A, 0x07F5, 0x167C, 0x24E7, 0x356E,
	0xCD99, 0xDC10, 0xEE8B, 0xFF02, 0x8BBD, 0x9A34, 0xA8AF, 0xB926,
	0x5AFA, 0x4B73, 0x79E8, 0x6861, 0x1CDE, 0x0D57, 0x3FCC, 0x2E45,
	0xD6B2, 0xC73B, 0xF5A0, 0xE429, 0x9096, 0x811F, 0xB384, 0xA20D,
	0x53E3, 0x426A, 0x70F1, 0x6178, 0x15C7, 0x044E, 0x36D5, 0x275C,
	0xDFAB, 0xCE22, 0xFCB9, 0xED30, 0x998F, 0x8806, 0xBA9D, 0xAB14,
	0x6CAC, 0x7D25, 0x4FBE, 0x5E37, 0x2A88, 0x3B01, 0x099A, 0x1813,
	0xE0E4, 0xF16D, 0xC3F6, 0xD27F, 0xA6C0, 0xB749, 0x85D2, 0x945B,
	0x65B5, 0x743C, 0x46A7, 0x572E, 0x2391, 0x3218, 0x0083, 0x110A,
	0xE9FD, 0xF874, 0xCAEF, 0xDB66, 0xAFD9, 0xBE50, 0x8CCB, 0x9D42,
	0x7E9E, 0x6F17, 0x5D8C, 0x4C05, 0x38BA, 0x2933, 0x1BA8, 0x0A21,
	0xF2D6, 0xE35F, 0xD1C4, 0xC04D, 0xB4F2, 0xA57B, 0x97E0, 0x8669,
	0x7787, 0x660E, 0x5495, 0x451C, 0x31A3, 0x202A, 0x12B1, 0x0338,
	0xFBCF, 0xEA46, 0xD8DD, 0xC954, 0xBDEB, 0xAC62, 0x9EF9, 0x8F70
};

__const uint16_t crc16(uint8_t byte, uint16_t crc){
	crc = (crc << 8) ^ crc16table[((crc >> 8) ^ byte)];
	return crc;
}

__const uint16_t crc16n(void* v, size_t size, uint16_t crc){
	uint8_t* bytes = v;
	while( size --> 0 ){
		crc = crc16(*bytes++, crc);
	}
	return crc;
}

int mth_approx_eq(double a, double b, double precision){
	return fabs(a-b) < precision ? 1 : 0;
}












