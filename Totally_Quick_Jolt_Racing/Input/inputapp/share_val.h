#ifndef SHARE_VAL_H
#define SHARE_VAL_H

typedef struct
{
    float angle_roue;
    float acceleration;
    float frein;
    int rapport;
} ShareVal;

void copyShareVal(ShareVal*);

#endif