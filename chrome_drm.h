/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice
 * (including the next paragraph) shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL VIA, S3 GRAPHICS, AND/OR
 * ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _CHROME_DRM_H_
#define _CHROME_DRM_H_


/* Driver specific ioctl number */
#define DRM_CHROME_ALLOCMEM                    0x00
#define DRM_CHROME_FREEMEM                     0x01
#define DRM_CHROME_FREE                        0x02
#define DRM_CHROME_ALLOCATE_EVENT_TAG          0x03
#define DRM_CHROME_FREE_EVENT_TAG              0x04
#define DRM_CHROME_ALLOCATE_APERTURE           0x05
#define DRM_CHROME_FREE_APERTURE               0x06
#define DRM_CHROME_ALLOCATE_VIDEO_MEM          0x07
#define DRM_CHROME_FREE_VIDEO_MEM              0x08
#define DRM_CHROME_WAIT_CHIP_IDLE              0x09
#define DRM_CHROME_PROCESS_EXIT                0x0A
#define DRM_CHROME_RESTORE_PRIMARY             0x0B
#define DRM_CHROME_FLUSH_CACHE                 0x0C
#define DRM_CHROME_INIT                        0x0D
#define DRM_CHROME_FLUSH                       0x0E
#define DRM_CHROME_CHECKVIDMEMSIZE             0x0F
#define DRM_CHROME_PCIEMEMCTRL                 0x10
#define DRM_CHROME_AUTH_MAGIC	            0x11
#define DRM_CHROME_GET_PCI_ID	            0x12
#define DRM_CHROME_INIT_JUDGE		    0x16
#define DRM_CHROME_DMA		    	    0x17
#define DRM_CHROME_QUERY_BRANCH     	    0x18
#define DRM_CHROME_REQUEST_BRANCH_BUF	    0x19
#define DRM_CHROME_BRANCH_BUF_FLUSH	    0x1A
/* GEM-interfaced TTM core parts */
#define DRM_CHROME_GEM_CREATE			    0x20
#define DRM_CHROME_GEM_MMAP			    0x21
#define DRM_CHROME_GEM_PREAD			    0x22
#define DRM_CHROME_GEM_PWRITE			    0x23
#define DRM_CHROME_GEM_SET_DOMAIN		    0x24
#define DRM_CHROME_GEM_FLUSH		        0x25
#define DRM_CHROME_GEM_WAIT       		    0x26
#define DRM_CHROME_SHADOW_INIT		        0x27
#define DRM_CHROME_GEM_CPU_GRAB		    0x28
#define DRM_CHROME_GEM_CPU_RELEASE  	    0x29
#define DRM_CHROME_GEM_LEAVE				0x2A
#define DRM_CHROME_GEM_ENTER				0x2B
#define DRM_CHROME_GEM_PIN					0x30
#define DRM_CHROME_GEM_UNPIN				0x31

/* The memory type for ttm */
#define CHROME_GEM_DOMAIN_CPU		0x1
#define CHROME_GEM_DOMAIN_GTT		0x2
#define CHROME_GEM_DOMAIN_VRAM		0x4
/* The memory flag for ttm */
#define CHROME_GEM_FLAG_NO_EVICT    (1 << 21)

/* VIA Chrome9 FLUSH flag*/
#define CHROME_FLUSH_RING_BUFFER		0x1
#define CHROME_FLUSH_BRANCH_BUFFER		0x2

/* VIA Chrome9 BO flags */
#define CHROME_BO_FLAG_CMD_FLUSHING	0x1

/* Define the R/W mode of each ioctl */
#define DRM_IOCTL_CHROME_GEM_CREATE	\
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_GEM_CREATE, \
	struct drm_chrome_gem_create)
#define DRM_IOCTL_CHROME_GEM_MMAP	\
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_GEM_MMAP, \
	struct drm_chrome_gem_mmap)
#define DRM_IOCTL_CHROME_GEM_PREAD	\
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_GEM_PREAD, \
	struct drm_chrome_gem_pread)
#define DRM_IOCTL_CHROME_GEM_PWRITE	\
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_GEM_PWRITE, \
	struct drm_chrome_gem_pwrite)
#define DRM_IOCTL_CHROME_GEM_SET_DOMAIN	\
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_GEM_SET_DOMAIN, \
	struct drm_chrome_gem_set_domain)
#define DRM_IOCTL_CHROME_GEM_FLUSH	\
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_GEM_FLUSH, \
	struct drm_chrome_gem_flush)
#define DRM_IOCTL_CHROME_GEM_WAIT	\
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_GEM_WAIT, \
	struct drm_chrome_gem_wait)
#define DRM_IOCTL_CHROME_ALLOCATE_EVENT_TAG \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_ALLOCATE_EVENT_TAG, \
	struct drm_chrome_event_tag)
#define DRM_IOCTL_CHROME_FREE_EVENT_TAG \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_FREE_EVENT_TAG, \
	struct drm_chrome_event_tag)
#define DRM_IOCTL_CHROME_GEM_CPU_GRAB \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_GEM_CPU_GRAB,\
	struct drm_chrome_cpu_grab)
#define DRM_IOCTL_CHROME_GEM_CPU_RELEASE \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_GEM_CPU_RELEASE,\
	struct drm_chrome_cpu_release)
#define DRM_IOCTL_CHROME_GEM_PIN \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_CHROME_GEM_PIN, \
	struct drm_chrome_gem_pin)
#define DRM_IOCTL_CHROME_GEM_UNPIN \
	DRM_IOW(DRM_COMMAND_BASE + DRM_CHROME_GEM_UNPIN, \
	struct drm_chrome_gem_unpin)
#define DRM_IOCTL_CHROME_GEM_LEAVE \
	DRM_IO(DRM_COMMAND_BASE + DRM_CHROME_GEM_LEAVE, int)
#define DRM_IOCTL_CHROME_GEM_ENTER \
	DRM_IO(DRM_COMMAND_BASE + DRM_CHROME_GEM_ENTER, int)

/* The data structure of each ioctl, which is used to
 * communicate between user mode and kernel mode
 */
struct drm_chrome_gem_create {
	uint64_t	size;
	uint64_t	alignment;
	uint32_t	handle;
	uint32_t	initial_domain;
	uint32_t	flags;
	uint32_t	offset;
};

struct drm_chrome_gem_mmap {
	uint32_t	handle;
	uint32_t	pad;
	uint64_t	offset;
	uint64_t	size;
	uint64_t	addr_ptr;
	void	*virtual;
};

struct drm_chrome_gem_pread {
	/** Handle for the object being read. */
	uint32_t handle;
	uint32_t pad;
	/** Offset into the object to read from */
	uint64_t offset;
	/** Length of data to read */
	uint64_t size;
	/** Pointer to write the data into. */
	/* void *, but pointers are not 32/64 compatible */
	uint64_t data_ptr;
};

struct drm_chrome_gem_pwrite {
	/** Handle for the object being written to. */
	uint32_t handle;
	uint32_t pad;
	/** Offset into the object to write to */
	uint64_t offset;
	/** Length of data to write */
	uint64_t size;
	/** Pointer to read the data from. */
	/* void *, but pointers are not 32/64 compatible */
	uint64_t data_ptr;
};

struct drm_chrome_gem_set_domain {
	uint32_t	handle;
	uint32_t	read_domains;
	uint32_t	write_domain;
};

struct drm_chrome_gem_wait {
	/* the buffer object handle */
	uint32_t handle;
	uint32_t no_wait;
};

struct drm_chrome_gem_flush {
	/* the pointer to this exec buffer */
	uint64_t buffer_ptr;
	uint32_t num_cliprects;
	/* This is a struct drm_clip_rect *cliprects */
	uint64_t cliprects_ptr;
	uint32_t flag;
};

enum drm_chrome_reloc_type {
	/* the reloc type for 2D command */
	CHROME_RELOC_2D,
	/* the reloc type for 3D command */
	CHROME_RELOC_3D,
	/* the reloc type for video*/
	CHROME_RELOC_VIDEO_HQV_SF,
	CHROME_RELOC_VIDEO_SL,
	CHROME_RELOC_VERTEX_STREAM_L,
	CHROME_RELOC_VERTEX_STREAM_H
};

struct drm_chrome_gem_relocation_entry {
	/* the target bo handle which add at the reloc list */
	uint32_t target_handle;
	/* the additional offset to this bo */
	uint32_t delta;
	/* this offset to command buffer */
	uint64_t offset;
	/* the target bo offset */
	uint64_t presumed_offset;
	/* the mask indicate this bo location */
	uint32_t location_mask;
	/* the command type for this target bo*/
	enum drm_chrome_reloc_type type;
	/* read write domain by this operation*/
	uint32_t	read_domains;
	uint32_t	write_domain;
};

struct drm_chrome_gem_exec_object {
	/* the command buffer pointer */
	union {
		uint64_t buffer_ptr;
		uint32_t cmd_bo_handle;
	};
	/* the reloc list pointer */
	uint64_t relocs_ptr;
	/* the buffer length we want to exec (dword) */
	uint32_t buffer_length;
	/* the bo count for reloc list */
	uint32_t relocation_count;
};

struct drm_chrome_shadow_init{
	/*for shadow & event tag*/
	unsigned int   shadow_size;
	unsigned long   shadow_handle;
};

struct drm_chrome_cpu_release {
	uint32_t   handle;
};

struct drm_chrome_cpu_grab {
	uint32_t handle;
	uint32_t flags;
};

/* @handle: bo handle
 * @location_mask: to which location bo pinned
 * @offset: the updated offset deliver to user
 * After pin, user mode process should fetch the updated
 * vram address from @offset
 */
struct drm_chrome_gem_pin {
	uint32_t handle;
	uint32_t location_mask;
	uint64_t offset;
};

struct drm_chrome_gem_unpin {
	uint32_t handle;
};

struct event_value {
	int event_low;
	int event_high;
};

struct drm_chrome_event_tag {
	unsigned int  event_size;         /* event tag size */
	int	 event_offset;                /* event tag id */
	struct event_value last_sent_event_value;
	struct event_value current_event_value;
	int         query_mask0;
	int         query_mask1;
	int         query_Id1;
};

extern int
chrome_ioctl_gem_create(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_gem_mmap(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_gem_pread(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_gem_pwrite(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_gem_set_domain(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_gem_wait(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_gem_flush(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_shadow_init(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_shadow_fini(struct drm_device *dev);
extern int
chrome_ioctl_alloc_event(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_free_event(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_wait_chip_idle(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int 
chrome_ioctl_cpu_grab(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_cpu_release(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_gem_pin(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int
chrome_ioctl_gem_unpin(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int chrome_ioctl_leave_gem(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
extern int chrome_ioctl_enter_gem(struct drm_device *dev, void *data,
	struct drm_file *file_priv);
int chrome_drm_resume(struct drm_device *dev);
int  chrome_drm_suspend(struct drm_device *dev, pm_message_t state);
void chrome_init_gart_table(struct drm_device *dev);
void set_agp_ring_buffer_stop(struct drm_device *dev);
void set_agp_ring_cmd_inv(struct drm_device *dev);
void chrome_preclose(struct drm_device *dev, struct drm_file *file_priv);
void chrome_lastclose(struct drm_device *dev);


#endif				/* _CHROME_DRM_H_ */
