/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#ifndef __X_LIBPD_MOD_UTILS_H__
#define __X_LIBPD_MOD_UTILS_H__

#ifdef __cplusplus


extern "C"
{
#endif
    
#include <z_libpd.h>
#include <m_pd.h>
    

    t_pd* libpd_createobj(t_pd *x, t_symbol *s, int argc, t_atom *argv);
    t_pd* libpd_creategraph(t_pd *x, int argc, t_atom *argv);

    void libpd_removeobj(t_canvas* cnv, t_gobj* obj);
    void libpd_renameobj(t_canvas* cnv, t_gobj* obj, const char* buf, int bufsize);
    void libpd_moveobj(t_canvas* cnv, t_gobj* obj, int x, int y);

    void libpd_createconnection(t_canvas* cnv, t_object*src, int nout, t_object*sink, int nin);
    void libpd_removeconnection(t_canvas* cnv, t_object*src, int nout, t_object*sink, int nin);
    
    void libpd_getcontent(t_canvas* cnv, char** buf, int* bufsize);

    t_pd* libpd_getobjectbyname(t_symbol *s, int argc, t_atom *argv);
    int libpd_type_exists(const char* type);

    int libpd_noutlets(const t_object *x);

    int libpd_ninlets(const t_object *x);

    int libpd_issignalinlet(const t_object *x, int m);
    int libpd_issignaloutlet(const t_object *x, int m);

    void libpd_canvas_doclear(t_canvas *x);

    void libpd_canvas_saveto(t_canvas *x, t_binbuf *b);

    void gobj_setposition(t_gobj *x, t_glist *glist, int xpos, int ypos);

    int libpd_tryconnect(t_canvas*x, t_object*src, int nout, t_object*sink, int nin);
    int libpd_canconnect(t_canvas*x, t_object*src, int nout, t_object*sink, int nin);

#ifdef __cplusplus
}
#endif

#endif
