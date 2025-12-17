#include <limits.h>

typedef int fixed_t;

/* 32bit x 32bit just using 16bit mulu */
fixed_t FixedMul(fixed_t a, fixed_t b)
{
	short sgn = 0;
	unsigned int t;
	
	if (a < 0)
	{
		a = -a;
		sgn ^= 1;
	}
	if (b < 0)
	{
		b = -b;
		sgn ^= 1;
	}

	t = ((unsigned short)(a & 0xffff)) * ((unsigned short)(b & 0xffff));
	t = (t >> 16) +
      (((unsigned short) (a & 0xffff)) * ((unsigned short) (b >> 16))) +
      (((unsigned short) (a >> 16)) * ((unsigned short) (b & 0xffff))) +
      ((((unsigned short) (a >> 16)) * ((unsigned short) (b >> 16))) << 16);
	t &= 0x7fffffff;

	return sgn ? -t : t;
}

fixed_t FixedDiv(fixed_t a, fixed_t b)
{
    short sgn = 0;
    register short i;
    register fixed_t res;
    fixed_t ah, al;


    /* A few tests */
    if (a == 0)
        return(0);
    if (b == 0)
        b = 0x10000;
    if (b == 0x10000)
        return(a);

    /* mess around with the signs */
    /* This breaks for minFix as unusally -minFix == minFix */
    if (a < 0)
    {
				sgn ^= 1;
        a = -a;
    }
    if (b < 0)
    {
				sgn ^= 1;
        b = -b;
    }
    if (((a >> 15) - b) >= 0)
    {
        res = 0x7fffffff;
    }
    else
    {
        ah = (a >> 14);
        al = (a << 18);
        res = 0;
        for (i = 31; i > 0; i--)
        {
            res = res << 1;
            if ((ah - b) >= 0)
            {
                ah -= b;
                res++;
            }
            ah = (ah << 1) + (al < 0 ? 1 : 0);
            al = al << 1;
        }
    }
    /* Fix up sign */
    return sgn ? -res : res;
}


