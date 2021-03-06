#include "perspective.h"

int cmpfunc(const void* a, const void* b) {
	return (*(int*)a - *(int*)b);
}

int PixelToCameraCoordinate(const CvPoint2D32f P[4], CvPoint2D32f C[4], const CvMat* K) {
	for (int i = 0; i < 4; i++) {
		//TODO: check fx and fy  this gives 6times in z for the output in abidi?
		float fx = CV_MAT_ELEM(*K, float, 0, 0);
		float fy = CV_MAT_ELEM(*K, float, 1, 1);
		float s  = CV_MAT_ELEM(*K, float, 0, 1);
		float cx = CV_MAT_ELEM(*K, float, 0, 2);
		float cy = CV_MAT_ELEM(*K, float, 1, 2);

		// apply inverse of K to pixel coordinates to get to normalized coordinates
		C[i].x = (P[i].x * fy - P[i].y * s - cx * fy + cy * s) / (fx * fy);
		C[i].y = (P[i].y - cy) / fy;
	}

	return EOK;
}

int GetP4PAbidi(const CvMat* S, const CvPoint2D32f P[4], CvPoint3D32f out[4], float* confidence) {
	float s12 = CV_MAT_ELEM(*S, float, 0, 1);
	float s13 = CV_MAT_ELEM(*S, float, 0, 2);
	float s14 = CV_MAT_ELEM(*S, float, 0, 3);
	float s23 = CV_MAT_ELEM(*S, float, 1, 2);
	float s24 = CV_MAT_ELEM(*S, float, 1, 3);
	float s34 = CV_MAT_ELEM(*S, float, 2, 3);
	float s41 = s14;

	// needs to be evaluated only once (constant)
	float A1 = sqrt(pow((pow(s12,2) + pow(s13,2) + pow(s23,2)),2) - 2 * (pow(s12,4) + pow(s13,4) + pow(s23,4))) / 4.0;
	float A2 = sqrt(pow((pow(s12,2) + pow(s14,2) + pow(s24,2)),2) - 2 * (pow(s12,4) + pow(s14,4) + pow(s24,4))) / 4.0;
	float A3 = sqrt(pow((pow(s13,2) + pow(s14,2) + pow(s34,2)),2) - 2 * (pow(s13,4) + pow(s14,4) + pow(s34,4))) / 4.0;
	float A4 = sqrt(pow((pow(s23,2) + pow(s24,2) + pow(s34,2)),2) - 2 * (pow(s23,4) + pow(s24,4) + pow(s34,4))) / 4.0;

	float x1 = P[0].x;
	float y1 = P[0].y;
	float x2 = P[1].x;
	float y2 = P[1].y;
	float x3 = P[2].x;
	float y3 = P[2].y;
	float x4 = P[3].x;
	float y4 = P[3].y;

	float B1 = x1 * (y3 - y2) + y1 * (x2 - x3) + y2 * x3 - x2 * y3;
	float B2 = x1 * (y4 - y2) + y1 * (x2 - x4) + y2 * x4 - x2 * y4;
	float B3 = x1 * (y4 - y3) + y1 * (x3 - x4) + y3 * x4 - x3 * y4;
	float B4 = x2 * (y4 - y3) + y2 * (x3 - x4) + y3 * x4 - x3 * y4;

	float C12 = (B3 * A4) / (A3 * B4);
	float C13 = (B2 * A4) / (A2 * B4);
	float C14 = (B1 * A4) / (A1 * B4);
	float C23 = (B2 * A3) / (A2 * B3);
	float C24 = (B1 * A3) / (A1 * B3);
	float C34 = (B1 * A2) / (A1 * B2);
	float C21 = 1.0 / C12;
	float C31 = 1.0 / C13;
	float C41 = 1.0 / C14;
	float C32 = 1.0 / C23;
	float C42 = 1.0 / C24;
	float C43 = 1.0 / C34;

	float H12s = pow((x1 - C12 * x2), 2) + pow((y1 - C12 * y2), 2);
	float H13s = pow((x1 - C13 * x3), 2) + pow((y1 - C13 * y3), 2);
	float H14s = pow((x1 - C14 * x4), 2) + pow((y1 - C14 * y4), 2);
	float H23s = pow((x2 - C23 * x3), 2) + pow((y2 - C23 * y3), 2);
	float H24s = pow((x2 - C24 * x4), 2) + pow((y2 - C24 * y4), 2);
	float H34s = pow((x3 - C34 * x4), 2) + pow((y3 - C34 * y4), 2);
	float H21s = pow((x2 - C21 * x1), 2) + pow((y2 - C21 * y1), 2);
	float H31s = pow((x3 - C31 * x1), 2) + pow((y3 - C31 * y1), 2);
	float H41s = pow((x4 - C41 * x1), 2) + pow((y4 - C41 * y1), 2);
	float H32s = pow((x3 - C32 * x2), 2) + pow((y3 - C32 * y2), 2);
	float H42s = pow((x4 - C42 * x2), 2) + pow((y4 - C42 * y2), 2);
	float H43s = pow((x4 - C43 * x3), 2) + pow((y4 - C43 * y3), 2);
	
	float f = 1;

	float F1 = sqrt(pow(x1, 2) + pow(y1, 2) + pow(f, 2));
	float F2 = sqrt(pow(x2, 2) + pow(y2, 2) + pow(f, 2));
	float F3 = sqrt(pow(x3, 2) + pow(y3, 2) + pow(f, 2));
	float F4 = sqrt(pow(x4, 2) + pow(y4, 2) + pow(f, 2));

	float d1s[6] = {
		s12* F1* pow(H12s + pow(f, 2) * pow((1 - C12), 2), -0.5),
		s13* F1* pow(H13s + pow(f, 2) * pow(1 - C13, 2), -0.5),
		s14* F1* pow(H14s + pow(f, 2) * pow(1 - C14, 2), -0.5),
		(s23 * F1 * pow(H23s + pow(f, 2) * pow(1 - C23, 2), -0.5)) / C12,
		(s24 * F1 * pow(H24s + pow(f, 2) * pow(1 - C24, 2), -0.5)) / C12,
		(s34 * F1 * pow(H34s + pow(f, 2) * pow(1 - C34, 2), -0.5)) / C13
	};

	// TODO: check if this sorting also always sorts the (x,y,z) values for the Ps (then only need to compute 2,3 and take average -> median)
	qsort(d1s, 6, sizeof(float), cmpfunc);

	float mean = 0.0;
	for (int i = 0; i < 6; i++) {
		mean += d1s[i];
	}
	mean /= 6.0;
	float variance = 0.0;
	for (int i = 0; i < 6; i++) {
		variance += pow(d1s[i] - mean, 2);
	}
	variance /= 6.0;
	float std_dev = sqrt(variance);

	CvPoint3D32f P1[6];
	CvPoint3D32f P2[6];
	CvPoint3D32f P3[6];
	CvPoint3D32f P4[6];

	for (int i = 2; i < 4; i++) {

		float d1 = d1s[i];
		float d2 = (C12 * F2 / F1) * d1;
		float d3 = (C13 * F3 / F1) * d1;
		float d4 = (C14 * F4 / F1) * d1;

		P1[i] = cvPoint3D32f((x1 / F1) * d1, (y1 / F1) * d1, f + (f / F1) * d1);
		P2[i] = cvPoint3D32f((x2 / F2) * d2, (y2 / F2) * d2, f + (f / F2) * d2);
		P3[i] = cvPoint3D32f((x3 / F3) * d3, (y3 / F3) * d3, f + (f / F3) * d3);
		P4[i] = cvPoint3D32f((x4 / F4) * d4, (y4 / F4) * d4, f + (f / F4) * d4);
	}

	out[0] = cvPoint3D32f((P1[2].x + P1[3].x) / 2, (P1[2].y + P1[3].y) / 2, (P1[2].z + P1[3].z) / 2);
	out[1] = cvPoint3D32f((P2[2].x + P2[3].x) / 2, (P2[2].y + P2[3].y) / 2, (P2[2].z + P2[3].z) / 2);
	out[2] = cvPoint3D32f((P3[2].x + P3[3].x) / 2, (P3[2].y + P3[3].y) / 2, (P3[2].z + P3[3].z) / 2);
	out[3] = cvPoint3D32f((P4[2].x + P4[3].x) / 2, (P4[2].y + P4[3].y) / 2, (P4[2].z + P4[3].z) / 2);

	*confidence = std_dev;

	return EOK;
}

int GetDistancesMatrix(CvMat* S) {
	float s12 = 154.5;
	float s13 = 219;
	float s14 = 154.5;
	float s15 = 126.5;
	float s16 = 888;
	float s17 = 1251;
	float s18 = 884;

	float s23 = 154.5;
	float s24 = 217.5;
	float s25 = 95.5;
	float s26 = 734.5;
	float s27 = 1146;
	float s28 = 896.5;

	float s34 = 153.5;
	float s35 = 96.5;
	float s36 = 749.5;
	float s37 = 1033.2;
	float s38 = 746.5;

	float s45 = 123.5;
	float s46 = 899.5;
	float s47 = 1145.5;
	float s48 = 729.5;

	float s56 = 793.5;
	float s57 = 1216;
	float s58 = 813.5;

	float s67 = 883;
	float s68 = 1251.748;

	float s78 = 884;

	CV_MAT_ELEM(*S, float, 0, 1) = s36;
	CV_MAT_ELEM(*S, float, 0, 2) = s37;
	CV_MAT_ELEM(*S, float, 0, 3) = s38;
	CV_MAT_ELEM(*S, float, 1, 2) = s67;
	CV_MAT_ELEM(*S, float, 1, 3) = s68;
	CV_MAT_ELEM(*S, float, 2, 3) = s78;

	return EOK;
}


int CalculateRotationMatrix(const CvPoint3D32f abidi[4], CvMat* R) {
	float x1 = abidi[0].x;
	float y1 = abidi[0].y;
	float z1 = abidi[0].z;
	float x2 = abidi[2].x;
	float y2 = abidi[2].y;
	float z2 = abidi[2].z;
	float x3 = abidi[3].x;
	float y3 = abidi[3].y;
	float z3 = abidi[3].z;

	float tx1 = 155.117  + abidi[1].x;
	float ty1 = -733.273 + abidi[1].y;
	float tz1 = -18.0    + abidi[1].z;
	float tx2 = 883.0    + abidi[1].x;
	float ty2 = 0.0      + abidi[1].y;
	float tz2 = -18.0    + abidi[1].z;
	float tx3 = 886.243  + abidi[1].x;
	float ty3 = -883.994 + abidi[1].y;
	float tz3 = -18.0    + abidi[1].z;

	float r1 = (ty1 * tz2 * x3 - ty1 * tz3 * x2 - ty2 * tz1 * x3 + ty2 * tz3 * x1 + ty3 * tz1 * x2 - ty3 * tz2 * x1) / (tx1 * ty2 * tz3 - tx1 * ty3 * tz2 - tx2 * ty1 * tz3 + tx2 * ty3 * tz1 + tx3 * ty1 * tz2 - tx3 * ty2 * tz1);
	float r2 = -(tx1 * tz2 * x3 - tx1 * tz3 * x2 - tx2 * tz1 * x3 + tx2 * tz3 * x1 + tx3 * tz1 * x2 - tx3 * tz2 * x1) / (tx1 * ty2 * tz3 - tx1 * ty3 * tz2 - tx2 * ty1 * tz3 + tx2 * ty3 * tz1 + tx3 * ty1 * tz2 - tx3 * ty2 * tz1);
	float r3 = (tx1 * ty2 * x3 - tx1 * ty3 * x2 - tx2 * ty1 * x3 + tx2 * ty3 * x1 + tx3 * ty1 * x2 - tx3 * ty2 * x1) / (tx1 * ty2 * tz3 - tx1 * ty3 * tz2 - tx2 * ty1 * tz3 + tx2 * ty3 * tz1 + tx3 * ty1 * tz2 - tx3 * ty2 * tz1);
	float r4 = (ty1 * tz2 * y3 - ty1 * tz3 * y2 - ty2 * tz1 * y3 + ty2 * tz3 * y1 + ty3 * tz1 * y2 - ty3 * tz2 * y1) / (tx1 * ty2 * tz3 - tx1 * ty3 * tz2 - tx2 * ty1 * tz3 + tx2 * ty3 * tz1 + tx3 * ty1 * tz2 - tx3 * ty2 * tz1);
	float r5 = -(tx1 * tz2 * y3 - tx1 * tz3 * y2 - tx2 * tz1 * y3 + tx2 * tz3 * y1 + tx3 * tz1 * y2 - tx3 * tz2 * y1) / (tx1 * ty2 * tz3 - tx1 * ty3 * tz2 - tx2 * ty1 * tz3 + tx2 * ty3 * tz1 + tx3 * ty1 * tz2 - tx3 * ty2 * tz1);
	float r6 = (tx1 * ty2 * y3 - tx1 * ty3 * y2 - tx2 * ty1 * y3 + tx2 * ty3 * y1 + tx3 * ty1 * y2 - tx3 * ty2 * y1) / (tx1 * ty2 * tz3 - tx1 * ty3 * tz2 - tx2 * ty1 * tz3 + tx2 * ty3 * tz1 + tx3 * ty1 * tz2 - tx3 * ty2 * tz1);
	float r7 = (ty1 * tz2 * z3 - ty1 * tz3 * z2 - ty2 * tz1 * z3 + ty2 * tz3 * z1 + ty3 * tz1 * z2 - ty3 * tz2 * z1) / (tx1 * ty2 * tz3 - tx1 * ty3 * tz2 - tx2 * ty1 * tz3 + tx2 * ty3 * tz1 + tx3 * ty1 * tz2 - tx3 * ty2 * tz1);
	float r8 = -(tx1 * tz2 * z3 - tx1 * tz3 * z2 - tx2 * tz1 * z3 + tx2 * tz3 * z1 + tx3 * tz1 * z2 - tx3 * tz2 * z1) / (tx1 * ty2 * tz3 - tx1 * ty3 * tz2 - tx2 * ty1 * tz3 + tx2 * ty3 * tz1 + tx3 * ty1 * tz2 - tx3 * ty2 * tz1);
	float r9 = (tx1 * ty2 * z3 - tx1 * ty3 * z2 - tx2 * ty1 * z3 + tx2 * ty3 * z1 + tx3 * ty1 * z2 - tx3 * ty2 * z1) / (tx1 * ty2 * tz3 - tx1 * ty3 * tz2 - tx2 * ty1 * tz3 + tx2 * ty3 * tz1 + tx3 * ty1 * tz2 - tx3 * ty2 * tz1);

	CV_MAT_ELEM(*R, float, 0, 0) = r1;
	CV_MAT_ELEM(*R, float, 0, 1) = r2;
	CV_MAT_ELEM(*R, float, 0, 2) = r3;
	CV_MAT_ELEM(*R, float, 1, 0) = r4;
	CV_MAT_ELEM(*R, float, 1, 1) = r5;
	CV_MAT_ELEM(*R, float, 1, 2) = r6;
	CV_MAT_ELEM(*R, float, 2, 0) = r7;
	CV_MAT_ELEM(*R, float, 2, 1) = r8;
	CV_MAT_ELEM(*R, float, 2, 2) = r9;

	return EOK;
}


int RotationMatrixToEuler(const CvMat* R, Euler* angles) {
	float r1 = CV_MAT_ELEM(*R, float, 0, 0);
	float r4 = CV_MAT_ELEM(*R, float, 1, 0);
	float r7 = CV_MAT_ELEM(*R, float, 2, 0);
	float r8 = CV_MAT_ELEM(*R, float, 2, 1);
	float r9 = CV_MAT_ELEM(*R, float, 2, 2);

	float sy = sqrt(pow(r1,2) + pow(r4,2));
	angles->z = atan2(r4, r1);
	angles->y = atan2(-r7, sy);
	angles->x = atan2(r8, r9);

	return EOK;
}


int RadiansToDegreesEulers(const Euler angles_rad, Euler* angles_deg) {
	angles_deg->x = angles_rad.x * 180.0 / M_PI;
	angles_deg->y = angles_rad.y * 180.0 / M_PI;
	angles_deg->z = angles_rad.z * 180.0 / M_PI;
}