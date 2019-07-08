/*
        plot.h

        @author Arvid Umrao <akumrao@yahoo.com> <arvind.umrao@harman.com>
        @version 0.1
 */
#ifndef PLOT
#define PLOT

#ifdef __ANDROID__
#include "SDL.h"
#define APP_NAME "web-plot"
#else
#include "SDL2/SDL.h"
#endif

#include "SDL_font.h"

//////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////

/// text inside box( for eg. load and download)
#define DOT_RADIUS                    6
#define CAPTION_OFFSET_CIRCLE_TO_TEXT 10
#define CAPTION_OFFSET_DELIMITER      40
#define GRADUATION_HEIGTH             10
//#define CAPTION_Y_LABEL_OFFSET        10
#define CAPTION_MARGIN                0



//-------------Plot SDL objects structure  ------------

typedef struct plot_struct {
    SDL_Window *screen;
    SDL_Renderer *renderer;

} splot;

/**
 * @brief plot_graph
 *      call draw_plot function, when exit event received clean everything
 * @param params
 *      plot parameters (cf plot_params struct)
 */

class Plot {
public:

    Plot();

    int plot_graph(Plot_Window_params *params, const char *title);

    static void mainloop(void *arg);

     static int run(void *arg);

    /**
     * @brief draw_scale_graduation
     *      draw graduation for the graph
     * @param renderer
     *      SDL renderer object
     * @param params
     *      plot parameters (cf plot_params struct)
     * @param plot
     *      structure containing textures and surfaces
     * @param plot_width
     *      plot base width (with proportion to screen width)
     * @param plot_heigth
     *      plot bas heigth (with proportion to screen heigth)
     * @param plot_mask_position
     *      SDL rectangle giving position of plot base (x,y) from max x or max y due to stroke width
     * @param font
     *      SDL font used for captions
     * @param font_color
     *      font color to be used
     * @param surface_list
     *      list of surfaces stored to be freed later
     * @param plot_position_x
     *      x position of plot
     * @param plot_position_y
     *      y position of plot
     */
    void draw_scale_graduation(SDL_Renderer * renderer,
            plot_params *params,
            splot *plot,
            SDL_Color font_color);

    /**
     * @brief draw_points
     *      draw each coordinate points on the graph featuring cartesian coordinate system
     * @param renderers
     *      SDL renderer
     * @param caption_item
     *      caption item structure
     * @param params
     *      plot parameters (cf plot_params struct)
     * @param plot_width
     *      plot base width (with proportion to screen width)
     * @param plot_heigth
     *      plot bas heigth (with proportion to screen heigth)
     * @param plot_mask_position
     *      SDL rectangle giving position of plot base (x,y) from max x or max y due to stroke width
     */
    void draw_points(SDL_Renderer* renderer, Caption_item* caption_item, plot_params *params );

    /**
     * @brief draw_plot
     *      create a new SDL window and plot grap with given parameters
     * @param splot
     *      plot containing SDL objects
     * @param params
     *      plot parameters (cf plot_params struct)
     * @param surface_list
     *      list of surfaces stored to be freed later
     */
    void draw_plot(splot *plot, plot_params *params);

    /**
     * @brief wait_for_sdl_event
     *      wait for an event
     */
    void wait_for_sdl_event();

    /**
     * @brief clean_plot
     *      full clean of SDL pointers and linked list clear
     * @param splot
     *      plot containing SDL items
     * @param params
     *      plot parameters (cf plot_params struct)
     * @param surface_list
     *      list of surfaces stored to be freed later
     */
    void clean_plot(splot *plot, plot_params *params);

    /*
     * from : http://content.gpwiki.org/index.php/SDL:Tutorials:Drawing_and_Filling_Circles
     * 
     * This is an implementation of the Midpoint Circle Algorithm 
     * found on Wikipedia at the following link:
     *
     *   http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
     *
     * The algorithm elegantly draws a circle quickly, using a
     * set_pixel function for clarity.
     */
    void draw_circle(SDL_Renderer *renderer, int n_cx, int n_cy, int radius);

    /*
     * from : http://content.gpwiki.org/index.php/SDL:Tutorials:Drawing_and_Filling_Circles
     *
     * SDL_Surface 32-bit circle-fill algorithm without using trig
     *
     * While I humbly call this "Celdecea's Method", odds are that the 
     * procedure has already been documented somewhere long ago.  All of
     * the circle-fill examples I came across utilized trig functions or
     * scanning neighbor pixels.  This algorithm identifies the width of
     * a semi-circle at each pixel height and draws a scan-line covering
     * that width.  
     *
     * The code is not optimized but very fast, owing to the fact that it
     * alters pixels in the provided surface directly rather than through
     * function calls.
     *
     * WARNING:  This function does not lock surfaces before altering, so
     * use SDL_LockSurface in any release situation.
     */
    void fill_circle(SDL_Renderer *renderer, int cx, int cy, int radius);

    void draw_window(splot *plot, Plot_Window_params *params);

private:

   

    splot plot;

    Plot_Window_params *win_params;

    int nWindows;

    int nHoriz;
    int nVert;
    
public:
    volatile static int keepRunning;

};


#endif