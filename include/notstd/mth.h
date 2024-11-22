#ifndef __NOTSTD_MATH_H__
#define __NOTSTD_MATH_H__

#include <stddef.h>
#include <complex.h>
#include <stdint.h>
#include <notstd/compiler.h>

typedef struct rndUnique{
	int      min;
	int      max;
	unsigned count;
	unsigned it;
	int*     buffer;
}rndUnique_s;

/** degree to radiant
 * @param gradi degree
 * @return radiant
 */
double mth_gtor(double gradi);

/** initialize random number*/
void mth_random_begin(void);

/** get random from 0 to N-1
 * @param n number
 * @return random
 */
int mth_random(int n);

/** get random in range, from min to max
 * @param min min value
 * @param max max value
 * @return random
 */
int mth_random_range(int min,int max);

/** get random gauss
 * @param media mediana value
 * @param stddeviation deviation
 * @return random
 */
float mth_random_gauss(float media, float stddeviation);

/** get random from 0.0 to 1.0
 * @return random
 */
double mth_random_f01(void);

/** get random string [a-zA-Z0-9] with size = size -1
 * @param size size string with 0
 * @param out output
 */
void mth_random_string_azAZ09(char* out, size_t size);

rndUnique_s* mth_random_unique_ctor(rndUnique_s* ru, int min, int max);
rndUnique_s* mth_random_unique_dtor(rndUnique_s* ru);
rndUnique_s* mth_random_unique_reset(rndUnique_s* ru);
int mth_random_unique(rndUnique_s* ru, int* extract);

/** rotate a point
 * @param x x position, out new position here
 * @param y y position, out new position here
 * @param centerx x rotation center
 * @param centery y rotation center
 * @param rad radiant rotation
 */
void mth_rotate(float *x,float *y,float centerx,float centery,float rad);

/** convert julian date to time_t
 * @param jd julian date
 * @return time_t date
 */
unsigned long mth_date_julian_time(double jd);

/** convert yy mm dd in julian date
 * @param year
 * @param month
 * @param day dd+hh/24.0
 * @return julian date
 */
double mth_date_julian(int year,int month,double day);

/** convert yy mm dd in julian date
 * @param d day
 * @param m month
 * @param y year
 * @return julian date at 12h UT(universal Time)
 */
int mth_date_julian_ut(int d,int m,int y);

/** get sun position by julian date
 * @param j julian
 * @return sun position
 */
double mth_sun_position(double j);

/** get moon position by julian date and sun position
 * @param j julian
 * @param ls sun
 * @return sun position
 */
double mth_moon_position(double j, double ls);

/** get moon phase by date 
 * @param year
 * @param month
 * @param day
 * @param hour
 * @param ip out value
 * @return moon phase
 */
double mth_moon_phase(int year,int month,int day, double hour, int* ip);

/** sum matrix a to b and return in r
 * @param r out
 * @param a in
 * @param b in
 * @param szr row count
 * @param szc col count
 */
void mth_mat_addi(int** r, int** a, int** b, size_t szr, size_t szc);

/** sub matrix a to b and return in r
 * @param r out
 * @param a in
 * @param b in
 * @param szr row count
 * @param szc col count
 */
void mth_mat_subi(int** r, int** a, int** b, size_t szr, size_t szc);

/** mul int to matrix return in r
 * @param r out
 * @param a integer
 * @param b in
 * @param szr row count
 * @param szc col count
 */
void mth_mat_imuli(int** r, int a, int** b, size_t szr, size_t szc);

/** mul matrix a to b and return in r
 * @param r out
 * @param a in
 * @param b in
 * @param szr row count
 * @param szc col count
 */
void mth_mat_muli(int** r, int** a, int** b, size_t szr, size_t szc);

/** determinat of matrix
 * @param a in
 * @return determinant
 */
int mth_mat_determinant2(int** a);

/** determinat3 of matrix
 * @param a in
 * @return determinant3
 */
int mth_mat_determinant3(int** a);

/** determinat of matrix
 * @param buffer data
 * @param samplerate samplerate
 * @param durata lenght
 * @param stfq startfq
 * @param loopfq loop
 * @param fq frequency
 * @param amplitude amplitude
 * @param fase fase
 */
void mth_fqr_generate(short int* buffer,int samplerate,double durata,double stfq,int loopfq,double fq,double amplitude,int fase);

/** simpple fft
 * @param buf in
 * @param n size buffer
 * @return fft result, free memory after use
 */
double complex* mth_fft(double complex buf[], int n);

/** calcolate pi
 * @param i
 * @return pi
 */
double mth_bbppigreco(long int i);

/** round number to up value as power of two
 * @param n is number
 */
__const size_t mth_round_up_power_two(size_t n);

/** International_System_of_Units*/
typedef enum{ MTH_SIP_ONE, MTH_SIP_DECA, MTH_SIP_HECTO, MTH_SIP_KILO, MTH_SIP_MEGA, MTH_SIP_GIGA, MTH_SIP_TERA, MTH_SIP_PETA, MTH_SIP_EXA, MTH_SIP_ZETTA, MTH_SIP_YOTTA } sip_e;

/** translate si to base*/
size_t mth_si_prefix_translate_base(const sip_e sibase);

/** translate to short string form*/
const char* mth_si_prefix_translate_short_string(const sip_e sibase);

/** move number to base
 * @param siout the si base
 * @param num number to translate
 * @return a base on si
 */
double mth_si_prefix_base(sip_e* siout, const double num);

/**  International Electrotechnical Commission */
typedef enum{ MTH_IEC_BYTE, MTH_IEC_KILOBYTE, MTH_IEC_MEGABYTE, MTH_IEC_GIGABYTE, MTH_IEC_TERABYTE, MTH_IEC_PETABYTE, MTH_IEC_EXABYTE, MTH_IEC_ZETTABYTE, MTH_IEC_YOTTABYTE } iecp_e;

/** translate iec to base*/
size_t mth_iec_prefix_translate_base(const iecp_e b);

/** translate to short string form*/
const char* mth_iec_prefix_translate_short_string(const iecp_e b);

/** move number to base
 * @param iecout the iec base
 * @param num number to translate
 * @return a base on iec
 */
double mth_ice_prefix_base(iecp_e* iecout, const double num);

/** encode data to base64 string
 * @param src data sources
 * @param size size of data
 * @return a new string or null if error, remember to free
 */
char* base64_encode(const void* src, const size_t size);

/** deencode string b64 to data
 * @param size out size of decoded data
 * @param b64 base64 string
 * @return a new data or null if error, remember to free
 */
void* base64_decode(size_t* size, const char* b64);

#define CRC16_INIT_VALUE (0xFFFF)

__const uint16_t crc16(uint8_t byte, uint16_t crc);
__const uint16_t crc16n(void* v, size_t size, uint16_t crc);

int mth_approx_eq(double a, double b, double precision);

#endif
