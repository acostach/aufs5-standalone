/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2005-2019 Junjiro R. Okajima
 */

/*
 * sub-routines for VFS
 */

#ifndef __AUFS_VFSUB_H__
#define __AUFS_VFSUB_H__

#ifdef __KERNEL__

#include <linux/fs.h>
#include <linux/mount.h>
#include "debug.h"

/* ---------------------------------------------------------------------- */

/* lock subclass for lower inode */
/* default MAX_LOCKDEP_SUBCLASSES(8) is not enough */
/* reduce? gave up. */
enum {
	AuLsc_I_Begin = I_MUTEX_PARENT2, /* 5 */
	AuLsc_I_PARENT,		/* lower inode, parent first */
	AuLsc_I_PARENT2,	/* copyup dirs */
	AuLsc_I_PARENT3,	/* copyup wh */
	AuLsc_I_CHILD,
	AuLsc_I_CHILD2,
	AuLsc_I_End
};

/* to debug easier, do not make them inlined functions */
#define MtxMustLock(mtx)	AuDebugOn(!mutex_is_locked(mtx))
#define IMustLock(i)		AuDebugOn(!inode_is_locked(i))

/* ---------------------------------------------------------------------- */

static inline void vfsub_drop_nlink(struct inode *inode)
{
	AuDebugOn(!inode->i_nlink);
	drop_nlink(inode);
}

static inline void vfsub_dead_dir(struct inode *inode)
{
	AuDebugOn(!S_ISDIR(inode->i_mode));
	inode->i_flags |= S_DEAD;
	clear_nlink(inode);
}

int vfsub_sync_filesystem(struct super_block *h_sb, int wait);

/* ---------------------------------------------------------------------- */

struct file *vfsub_dentry_open(struct path *path, int flags);
struct file *vfsub_filp_open(const char *path, int oflags, int mode);
int vfsub_kern_path(const char *name, unsigned int flags, struct path *path);

struct dentry *vfsub_lookup_one_len_unlocked(const char *name,
					     struct dentry *parent, int len);
struct dentry *vfsub_lookup_one_len(const char *name, struct dentry *parent,
				    int len);

struct vfsub_lkup_one_args {
	struct dentry **errp;
	struct qstr *name;
	struct dentry *parent;
};

static inline struct dentry *vfsub_lkup_one(struct qstr *name,
					    struct dentry *parent)
{
	return vfsub_lookup_one_len(name->name, parent, name->len);
}

void vfsub_call_lkup_one(void *args);

/* ---------------------------------------------------------------------- */

static inline int vfsub_mnt_want_write(struct vfsmount *mnt)
{
	int err;

	lockdep_off();
	err = mnt_want_write(mnt);
	lockdep_on();
	return err;
}

static inline void vfsub_mnt_drop_write(struct vfsmount *mnt)
{
	lockdep_off();
	mnt_drop_write(mnt);
	lockdep_on();
}

/* ---------------------------------------------------------------------- */

struct au_hinode;
struct dentry *vfsub_lock_rename(struct dentry *d1, struct au_hinode *hdir1,
				 struct dentry *d2, struct au_hinode *hdir2);
void vfsub_unlock_rename(struct dentry *d1, struct au_hinode *hdir1,
			 struct dentry *d2, struct au_hinode *hdir2);

int vfsub_create(struct inode *dir, struct path *path, int mode,
		 bool want_excl);
int vfsub_symlink(struct inode *dir, struct path *path,
		  const char *symname);
int vfsub_mknod(struct inode *dir, struct path *path, int mode, dev_t dev);
int vfsub_link(struct dentry *src_dentry, struct inode *dir,
	       struct path *path, struct inode **delegated_inode);
int vfsub_rename(struct inode *src_hdir, struct dentry *src_dentry,
		 struct inode *hdir, struct path *path,
		 struct inode **delegated_inode, unsigned int flags);
int vfsub_mkdir(struct inode *dir, struct path *path, int mode);
int vfsub_rmdir(struct inode *dir, struct path *path);

/* ---------------------------------------------------------------------- */

ssize_t vfsub_read_u(struct file *file, char __user *ubuf, size_t count,
		     loff_t *ppos);
ssize_t vfsub_read_k(struct file *file, void *kbuf, size_t count,
			loff_t *ppos);
ssize_t vfsub_write_u(struct file *file, const char __user *ubuf, size_t count,
		      loff_t *ppos);
ssize_t vfsub_write_k(struct file *file, void *kbuf, size_t count,
		      loff_t *ppos);
int vfsub_iterate_dir(struct file *file, struct dir_context *ctx);

static inline loff_t vfsub_f_size_read(struct file *file)
{
	return i_size_read(file_inode(file));
}

static inline unsigned int vfsub_file_flags(struct file *file)
{
	unsigned int flags;

	spin_lock(&file->f_lock);
	flags = file->f_flags;
	spin_unlock(&file->f_lock);

	return flags;
}

static inline int vfsub_file_execed(struct file *file)
{
	/* todo: direct access f_flags */
	return !!(vfsub_file_flags(file) & __FMODE_EXEC);
}

#if 0 /* reserved */
static inline void vfsub_touch_atime(struct vfsmount *h_mnt,
				     struct dentry *h_dentry)
{
	struct path h_path = {
		.dentry	= h_dentry,
		.mnt	= h_mnt
	};
	touch_atime(&h_path);
}
#endif

/*
 * re-use branch fs's ioctl(FICLONE) while aufs itself doesn't support such
 * ioctl.
 */
static inline loff_t vfsub_clone_file_range(struct file *src, struct file *dst,
					    loff_t len)
{
	loff_t err;

	lockdep_off();
	err = vfs_clone_file_range(src, 0, dst, 0, len, /*remap_flags*/0);
	lockdep_on();

	return err;
}

/* ---------------------------------------------------------------------- */

static inline loff_t vfsub_llseek(struct file *file, loff_t offset, int origin)
{
	loff_t err;

	lockdep_off();
	err = vfs_llseek(file, offset, origin);
	lockdep_on();
	return err;
}

/* ---------------------------------------------------------------------- */

int vfsub_sio_mkdir(struct inode *dir, struct path *path, int mode);
int vfsub_sio_rmdir(struct inode *dir, struct path *path);
int vfsub_sio_notify_change(struct path *path, struct iattr *ia,
			    struct inode **delegated_inode);
int vfsub_notify_change(struct path *path, struct iattr *ia,
			struct inode **delegated_inode);
int vfsub_unlink(struct inode *dir, struct path *path,
		 struct inode **delegated_inode, int force);

static inline int vfsub_getattr(const struct path *path, struct kstat *st)
{
	return vfs_getattr(path, st, STATX_BASIC_STATS, AT_STATX_SYNC_AS_STAT);
}

#endif /* __KERNEL__ */
#endif /* __AUFS_VFSUB_H__ */
