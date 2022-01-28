#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct buffer_object {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t handle;
    uint32_t size;
    uint8_t *vaddr;
    uint32_t fb_id;
};

struct buffer_object buf;

static int modeset_create_fb(int fd, struct buffer_object *bo) {
    struct drm_mode_create_dumb create = {};
    struct drm_mode_map_dumb map = {};

    create.width = bo->width;
    create.height = bo->height;
    create.bpp = 32;
    drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);

    bo->pitch = create.pitch;
    bo->size = create.size;
    bo->handle = create.handle;
    drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
                 bo->handle, &bo->fb_id);

    map.handle = create.handle;
    drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);

    bo->vaddr = static_cast<uint8_t *>(mmap(0, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map.offset));

    memset(bo->vaddr, 0x99, bo->size);

    return 0;
}

static void modeset_destroy_fb(int fd, struct buffer_object *bo) {
    struct drm_mode_destroy_dumb destroy = {};

    drmModeRmFB(fd, bo->fb_id);

    munmap(bo->vaddr, bo->size);

    destroy.handle = bo->handle;
    drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

int main(int argc, char **argv) {
    int fd;
    drmModeConnector *conn;
    drmModeRes *res;
    drmModePlaneRes *plane_res;
    uint32_t conn_id;
    uint32_t crtc_id;
    uint32_t plane_id;

    fd = open("/dev/dri/card1", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("/dev/dri/card1");
        return 1;
    }
    res = drmModeGetResources(fd);
    crtc_id = res->crtcs[0];
    for (int i = 0; i < res->count_connectors; ++i) {
        drmModeConnector *drm_conn = drmModeGetConnector(fd, res->connectors[i]);
        if (!drm_conn) {
            perror("drmModeGetConnector");
            continue;
        }
        if (drm_conn->connection == DRM_MODE_CONNECTED) {
            conn_id = res->connectors[i];
            printf("first connected idx:%d\n", i);
            break;
        }
    }


    drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    plane_res = drmModeGetPlaneResources(fd);
    printf("count_planes:%d\n", plane_res->count_planes);
    for (int i = 0; i < plane_res->count_planes; i++) {
        printf("%d ", plane_res->planes[i]);
    }
    printf("\n");
//    plane_id = plane_res->planes[3];


    for (int i = 0; i < plane_res->count_planes; i++) {
        plane_id = plane_res->planes[i];
        drmModePlanePtr plane = drmModeGetPlane(fd, plane_id);
        if (!plane) {
            continue;
        }
        int k;
        for (k = 0; k < res->count_crtcs; k++) {
            uint32_t bit = 1 << k;
            if ((plane->possible_crtcs & bit) == 0)continue;
            printf("plane match crtcs index:%d -> %d plane crtc_id:%d\n", k, plane_id, plane->crtc_id);
//            res->crtcs[k];
            break;
        }
        drmModeFreePlane(plane);
        if (k < res->count_crtcs) {
            break;
        }
    }

    conn = drmModeGetConnector(fd, conn_id);
    buf.width = conn->modes[0].hdisplay;
    buf.height = conn->modes[0].vdisplay;
    printf("widht:%d height:%d\n", buf.width, buf.height);
    modeset_create_fb(fd, &buf);
    printf("crtc_id:%d\n",crtc_id);
    drmModeCrtcPtr saved = drmModeGetCrtc(fd, crtc_id);

    drmModeSetCrtc(fd, crtc_id, buf.fb_id,
                   0, 0, &conn_id, 1, &conn->modes[0]);

    getchar();
    getchar();
//    for (int i = 0; i < plane_res->count_planes; i++) {
//        plane_id = plane_res->planes[i];
        plane_id = 45;
        printf("plane_id:%d crtc_id:%d\n", plane_id, crtc_id);
        /* crop the rect from framebuffer(100, 150) to crtc(50, 50) */
        int ret = drmModeSetPlane(fd, plane_id, crtc_id, buf.fb_id, 0,
                                  50, 50, 140, 140,
                                  100 << 16, 150 << 16, 140 << 16, 140 << 16);
        if (ret < 0) {
            perror("drmModeSetPlane");
        } /*else {
            break;
        }*/
//    }
    getchar();
    getchar();
    modeset_destroy_fb(fd, &buf);
    drmModeCrtc *crtc = saved;
    if (crtc) {
        drmModeSetCrtc(fd, crtc->crtc_id, crtc->buffer_id,
                       crtc->x, crtc->y, &conn_id, 1, &crtc->mode);
        drmModeFreeCrtc(crtc);
    }
    drmModeFreeConnector(conn);
    drmModeFreePlaneResources(plane_res);
    drmModeFreeResources(res);

    close(fd);

    return 0;
}
