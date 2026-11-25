#ifndef _IN_MODELPACKEMBED_H
#define _IN_MODELPACKEMBED_H

/**
 * A mesh that ships inside the executable instead of in model-packs/. The
 * generated table comes from tools/modelpack/embed.py; modelpack.c mounts the
 * files in a memory directory so objmeshRead() and pngRead() can open them by
 * the same names a pack would use.
 */
struct embeddedmodelfile {
	const char *name;
	const unsigned char *data;
	unsigned int len;
};

extern const struct embeddedmodelfile g_EmbeddedModelFiles[];
extern const int g_EmbeddedModelNumFiles;

#endif
