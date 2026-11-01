/* Key binding functions */
static void defaultgaps(const Arg *arg);
static void incrgaps(const Arg *arg);
static void incrigaps(const Arg *arg);
static void togglegaps(const Arg *arg);
static void tile(Monitor *m);

/* Internals */
static void getgaps(Monitor *m, int *oh, int *ov, int *ih, int *iv, unsigned int *nc);
static void setgaps(int oh, int ov, int ih, int iv);

/* Settings */
#if !PERTAG_PATCH
static int enablegaps = 1;
#endif // PERTAG_PATCH

void
setgaps(int oh, int ov, int ih, int iv)
{
	if (oh < 0) oh = 0;
	if (ov < 0) ov = 0;
	if (ih < 0) ih = 0;
	if (iv < 0) iv = 0;

	selmon->gappoh = oh;
	selmon->gappov = ov;
	selmon->gappih = ih;
	selmon->gappiv = iv;
	arrange(selmon);
}

void
togglegaps(const Arg *arg)
{
	#if PERTAG_PATCH
	selmon->pertag->enablegaps[selmon->pertag->curtag] = !selmon->pertag->enablegaps[selmon->pertag->curtag];
	#else
	enablegaps = !enablegaps;
	#endif // PERTAG_PATCH
	arrange(NULL);
}

void
defaultgaps(const Arg *arg)
{
	setgaps(gappoh, gappov, gappih, gappiv);
}

void
incrgaps(const Arg *arg)
{
	setgaps(
		selmon->gappoh + arg->i,
		selmon->gappov + arg->i,
		selmon->gappih + arg->i,
		selmon->gappiv + arg->i
	);
}

void
incrigaps(const Arg *arg)
{
	setgaps(
		selmon->gappoh,
		selmon->gappov,
		selmon->gappih + arg->i,
		selmon->gappiv + arg->i
	);
}

void
getgaps(Monitor *m, int *oh, int *ov, int *ih, int *iv, unsigned int *nc)
{
	unsigned int n, oe, ie;
	#if PERTAG_PATCH
	oe = ie = selmon->pertag->enablegaps[selmon->pertag->curtag];
	#else
	oe = ie = enablegaps;
	#endif // PERTAG_PATCH
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (smartgaps && n == 1) {
		oe = 0; // outer gaps disabled when only one client
	}

	*oh = m->gappoh*oe; // outer horizontal gap
	*ov = m->gappov*oe; // outer vertical gap
	*ih = m->gappih*ie; // inner horizontal gap
	*iv = m->gappiv*ie; // inner vertical gap
	*nc = n;            // number of clients
}

/*
 * Default tile layout + gaps (Exact Integer Engine)
 */
static void
tile(Monitor *m)
{
	unsigned int i, n;
	int oh, ov, ih, iv;
	int mx = 0, my = 0, mh = 0, mw = 0;
	int sx = 0, sy = 0, sh = 0, sw = 0;
	int rem_mh = 0, rem_mw = 0;
	int rem_sh = 0, rem_sw = 0;
	Client *c;

	getgaps(m, &oh, &ov, &ih, &iv, &n);
	if (n == 0)
		return;

	sx = mx = m->wx + ov;
	sy = my = m->wy + oh;
	mh = m->nmaster ? m->wh - 2*oh - ih * (MIN(n, (unsigned int)m->nmaster) - 1) : 0;
	sh = (n > (unsigned int)m->nmaster) ? m->wh - 2*oh - ih * (n - m->nmaster - 1) : 0;
	if (mh < 0) mh = 0;
	if (sh < 0) sh = 0;
	sw = mw = m->ww - 2*ov;

	if (m->nmaster && n > m->nmaster) {
		int total_w = mw - iv;
		int mfact_scaled = (int)(m->mfact * 1000.0f + 0.5f);
		mw = (total_w * mfact_scaled) / 1000;
		sw = total_w - mw;
		sx = mx + mw + iv;
	}

	/* Exact cfacts weights calculation */
	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++) {
		if (i < m->nmaster)
			rem_mw += c->cfact;
		else
			rem_sw += c->cfact;
	}
	rem_mh = mh;
	rem_sh = sh;

	/* Stepwise integer allocation: 0-pixel remainder drift, zero underflow */
	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++) {
		if (i < m->nmaster) {
			int h = (rem_mw > 0) ? (rem_mh * c->cfact) / rem_mw : 0;
			rem_mh -= h;
			rem_mw -= c->cfact;
			int win_w = MAX(1, mw - 2 * c->bw);
			int win_h = MAX(1, h - 2 * c->bw);
			resize(c, mx, my, win_w, win_h, 0);
			my += h + ih;
		} else {
			int h = (rem_sw > 0) ? (rem_sh * c->cfact) / rem_sw : 0;
			rem_sh -= h;
			rem_sw -= c->cfact;
			int win_w = MAX(1, sw - 2 * c->bw);
			int win_h = MAX(1, h - 2 * c->bw);
			resize(c, sx, sy, win_w, win_h, 0);
			sy += h + ih;
		}
	}
}
