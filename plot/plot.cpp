
/**
        plot.cpp

        create a new SDL window, plot points in a created graph featuring a cartesian coordinate system


        @author Arvid Umrao <akumrao@yahoo.com> <arvind.umrao@harman.com>
        @version 0.1
 */
#include "stdio.h"
#include "plot.h"

#ifdef __ANDROID__
#include "SDL.h"
//#	include "SDL_ttf.h"
#include "android/log.h"
#else
#include "SDL2/SDL.h"
#include "SDL2/SDL_thread.h"
//#	include "SDL2/SDL_ttf.h"
#endif

#include "math.h"
#include "SDL_font.h"



#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

volatile int Plot::keepRunning = 0;

int Plot::run(void *arg) {

    keepRunning = 2;
    while (keepRunning > 1) {

        mainloop(arg);
        SDL_Delay(1000);
    }

    keepRunning = 0;
}

void Plot::mainloop(void *arg) {
    Plot *ctx = static_cast<Plot*> (arg);
    
    ctx->draw_window(&ctx->plot, ctx->win_params);

/*
    static int x = 0;
    x = ++x % 5;

 

    plot_params *params = ctx->win_params->nxt->plotparm;
    params->clean();
    ctx->win_params->plotparm->update = true;
    params->update = true;
    params->push_back(0, -5, -2+x);
    params->push_back(0, -4, -9);
    params->push_back(0, -3, -9);
    params->push_back(0, -2, -3);
    params->push_back(0, -1, -8);
    params->push_back(0, 0, 0);
    params->push_back(0, 0, 9);
    params->push_back(0, 1, 5);
    params->push_back(0, 2, 12);
    params->push_back(0, 3, 18);
    params->push_back(0, 4, 3);


    plot_params *params1 = ctx->win_params->nxt->nxt->plotparm;
    params1->clean();
    params1->update = true;
    for (int i = 0; i < 2048; ++i) {

        float sineStep = 2 * M_PI * i * 440 / 44100;
         float ret;
        if(x < 4)
         ret=  x + (120 * sin(sineStep)) + 128;
        else
         ret=  x + (120 * cos(sineStep)) + 128;        
        params1->push_back(0, i, ret);
    }

    */
    return;



}

/**
 * @brief plot_graph
 *      create a new SDL window and plot grap with given parameters
 * @param params
 *      plot parameters (cf plot_params struct)
 */
Plot::Plot() : nWindows(0) {

    setvbuf(stdout, NULL, _IONBF, 0);

    //surfacelist surface_list = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) == -1) {

        myPrintf("Error SDL init failure : %s\n", SDL_GetError());
    }

    plot.screen = NULL;

    plot.renderer = NULL;


}

int Plot::plot_graph(Plot_Window_params *win_params, const char *title) {

    if(!win_params) {
        
        myPrintf("No graph to plot");
        return -1;
    }
    
    
    nWindows = win_params->count();

    plot_params *params = win_params->plotparm;
    this->win_params = win_params;

    nHoriz = nWindows;
    nVert = nWindows;
    switch (nWindows) {

        case 1:

            break;

        case 2:
            nVert = nVert / 2;
            break;

        case 3:
        case 4:
            nVert = 2;
            nHoriz = 2;
            break;

        case 5:
        case 6:
            nVert = 2;
            nHoriz = 3;
            break;

        default:
            myPrintf("Error, Plot Screens supported 1 - 8, you you screens %d\n", nWindows);
            return -1;
    };



#ifdef __EMSCRIPTEN__
    //emscripten_cancel_main_loop();
    // SDL_Delay(1500);
    SDL_CreateWindowAndRenderer(params->screen_width* nHoriz, params->screen_heigth*nVert, 0, &plot.screen, &plot.renderer);
    //------------ background-----------------------
    SDL_SetRenderDrawColor(plot.renderer, 255, 255, 255, 255);
    /* Clear the entire screen to our white color. */
    SDL_RenderClear(plot.renderer);
    const int simulate_infinite_loop = 1; // call the function repeatedly
    const int fps = -1; // call the function as fast as the browser wants to render (typically 60fps)
    emscripten_set_main_loop_arg(&Plot::mainloop, this, fps, simulate_infinite_loop);

#else


    plot.screen = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            params->screen_width* nHoriz,
            params->screen_heigth*nVert,
            SDL_WINDOW_SHOWN);

    //SDL_SetWindowFullscreen(plot.screen,SDL_WINDOW_FULLSCREEN);

    plot.renderer = SDL_CreateRenderer(plot.screen, 0, 0);

    //------------ background-----------------------
    SDL_SetRenderDrawColor(plot.renderer, 255, 255, 255, 255);
    /* Clear the entire screen to our white color. */
    SDL_RenderClear(plot.renderer);
    wait_for_sdl_event();
#endif


    clean_plot(&plot, params);

    return EXIT_SUCCESS;
}

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
void Plot::clean_plot(splot *plot, plot_params *params) {

    params->caption_list = clear_caption(params->caption_list);
    params->coordinate_list = clear_coord(params->coordinate_list);

    // *surface_list = clear_surface(*surface_list);

    SDL_DestroyRenderer(plot->renderer);
    SDL_DestroyWindow(plot->screen);

    SDL_Quit();

}

void Plot::draw_window(splot *plot, Plot_Window_params *win_params) {

    if (plot->screen != NULL) {


        int w;
        int h;

        int row = 0;
        int col = 0;
        int count = 0;
        while (win_params) {

            if (win_params->plotparm->update) {

                win_params->plotparm->update = false;

                w = win_params->plotparm->screen_width;
                h = win_params->plotparm->screen_heigth;
                SDL_Texture *parentTarget = SDL_GetRenderTarget(plot->renderer);

                if (!win_params->plotparm->texTarget)
                    win_params->plotparm->texTarget = SDL_CreateTexture(plot->renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, w, h);

                SDL_Rect srect = {0, 0, w, h};
                SDL_SetRenderTarget(plot->renderer, win_params->plotparm->texTarget);
                SDL_SetRenderDrawColor(plot->renderer, 255, 255, 255, 255);
                /* Clear the entire screen to our white color. */
                SDL_RenderClear(plot->renderer);
                //SDL_SetTextureBlendMode(texTarget, SDL_BLENDMODE_BLEND);
                /////////////////////////////////////////////////
                /// Draw one plot
                draw_plot(plot, win_params->plotparm);
                //// End one plot
                /////////////////////////////////////////////////


                //Now render the texture target to our screen, but upside down
                SDL_SetRenderTarget(plot->renderer, parentTarget);

                SDL_Rect drect = {col*w, row*h, w, h};
                
                win_params->plotparm->colPos= col*w;
                win_params->plotparm->rowPos= row*h;
        
                myPrintf(" win=%d, col = %d, row=%d \n", count, col, row);
                SDL_RenderCopy(plot->renderer, win_params->plotparm->texTarget, &srect, &drect);
                

            }

            ++count;

            row = (int) (count) / nHoriz;
            col = count % nHoriz;
            for(auto& kv : win_params->plotparm->points)
            {
                if(kv.second.size())
                {
                    SDL_RenderDrawLines(plot->renderer, &kv.second[0], kv.second.size()); 
                   // kv.second.clear();
                }

            }
            
            //   SDL_DestroyTexture(texTarget);
            win_params = win_params->nxt;

        }

        
        SDL_RenderPresent(plot->renderer);

    }
}

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
void Plot::draw_plot(splot *plot, plot_params *params) {


    if (plot->screen != NULL) {

        int stroke_width = 0;

        SDL_Color font_color = {0, 0, 0, 255};
        //---------------------------------------------

        SDL_Rect scree_position;
        scree_position.x = 0;
        scree_position.y = 0;
        scree_position.w = params->screen_width;
        scree_position.h = params->screen_heigth;


        SDL_SetRenderDrawColor(plot->renderer, 0, 255, 0, 255);
        SDL_RenderDrawRect(plot->renderer, &scree_position);


        SDL_SetRenderDrawColor(plot->renderer, 0, 255, 0, 255);
        SDL_RenderDrawRect(plot->renderer, &scree_position);

        // SDL_SetRenderDrawColor(plot->renderer, 255, 255, 0, 255);
        //  SDL_RenderDrawRect(plot->renderer, &plot_caption_position);
        //  SDL_RenderFillRect(plot->renderer, &plot_caption_position);


        //SDL_Rect caption_y_position;
        //  caption_y_position.x = plot_position.x;
        // caption_y_position.y = plot_position.y;


        draw_scale_graduation(plot->renderer,
                params,
                plot,
                font_color);

        if (params->caption_list != NULL) {
            Caption_item *tmp = params->caption_list;

            if (tmp != NULL) {
                int caption_offset = CAPTION_MARGIN;

                while (tmp != NULL) {
                    //plot cercle1
                    int circle_x1 = params->plot_caption_position.x + caption_offset;
                    int circle_y1 = params->plot_caption_position.y + params->plot_caption_position.h / 2 + stroke_width;

                    SDL_SetRenderDrawColor(plot->renderer, 0, 0, 0, 255);
                    fill_circle(plot->renderer, circle_x1, circle_y1, DOT_RADIUS);

                    SDL_SetRenderDrawColor(plot->renderer, (tmp->caption_color & 0xFF0000) >> 16,
                            (tmp->caption_color & 0x00FF00) >> 8,
                            tmp->caption_color & 0x0000FF, 255);

                    fill_circle(plot->renderer, circle_x1, circle_y1, DOT_RADIUS - 2);

                    //plot cercle2
                    caption_offset += 40;

                    int circle_x2 = params->plot_caption_position.x + caption_offset;
                    int circle_y2 = circle_y1;

                    SDL_SetRenderDrawColor(plot->renderer, 0, 0, 0, 255);
                    fill_circle(plot->renderer, circle_x2, circle_y2, DOT_RADIUS);

                    SDL_SetRenderDrawColor(plot->renderer, (tmp->caption_color & 0xFF0000) >> 16,
                            (tmp->caption_color & 0x00FF00) >> 8,
                            tmp->caption_color & 0x0000FF, 255);

                    fill_circle(plot->renderer, circle_x2, circle_y2, DOT_RADIUS - 2);

                    //draw line between two circles
                    SDL_RenderDrawLine(plot->renderer, circle_x1 + DOT_RADIUS + 1, circle_y1, circle_x2 - DOT_RADIUS - 1, circle_y2);

                    //display caption
                    //SDL_Surface *caption_text_surface = TTF_RenderText_Blended(plot->font, tmp->caption_txt, font_color);
                    SDL_Rect caption_text;
                    //SDL_Texture * texture_text = SDL_CreateTextureFromSurface(plot->renderer, caption_text_surface);
                    //SDL_QueryTexture(texture_text, NULL, NULL, &caption_text.w, &caption_text.h);
                    caption_text.w = FONT_CHARACTER_SIZE * strlen(tmp->caption_txt);
                    caption_text.h = FONT_CHARACTER_SIZE + 2;
                    caption_text.x = circle_x2 + DOT_RADIUS + CAPTION_OFFSET_CIRCLE_TO_TEXT;
                    caption_text.y = circle_y2 - caption_text.h / 2;
                    //  SDL_RenderCopy(plot->renderer, texture_text, NULL, &caption_text);

                    SDL_DrawString(plot->renderer, caption_text.x, caption_text.y, tmp->caption_txt, 8, &font_color);

                    // *surface_list = push_back_surface(*surface_list, caption_text_surface);

                    caption_offset += caption_text.w + DOT_RADIUS + CAPTION_OFFSET_CIRCLE_TO_TEXT + CAPTION_OFFSET_DELIMITER;

                    draw_points(plot->renderer,
                            tmp,
                            params);

                    tmp = tmp->nxt;
                }
            }
        }



#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_VERBOSE, APP_NAME, "plot finished");
#endif


    } else {
        fprintf(stderr, "Error cant allocate memory for screen : %s\n", SDL_GetError());
    }
}

/**
 * @brief draw_points
 *      draw each coordinate points on the graph featuring cartesian coordinate system
 * @param renderers
 *      SDL renderer object
 * @param caption_item
 *      caption item object structure
 * @param params
 *      plot parameters (cf plot_params struct)
 * @param  params->plot_position.w
 *      plot base width (with proportion to screen width)
 * @param  params->plot_position.h
 *      plot bas heigth (with proportion to screen heigth)
 * @param params->plot_position
 *      SDL rectangle giving position of plot base (x,y) from max x or max y due to stroke width
 */
void Plot::draw_points(
        SDL_Renderer* renderer,
        Caption_item* caption_item,
        plot_params *params) {
    Coordinate_item* tmp = params->coordinate_list;

    // float scale_x_num =  params->plot_position.w / (params->max.x / params->scale.x);
    // float scale_y_num =  params->plot_position.h / (params->max.y / params->scale.y);
    float scale_x_num = params->plot_position.w / ((params->max.x - params->min.x) / params->scale.x);
    float scale_y_num = params->plot_position.h / ((params->max.y - params->min.y) / params->scale.y);

    unsigned char isFirst = 1;

    float previous_x = 0;
    float previous_y = 0;


      
    while (tmp != NULL) {
        if (tmp->caption_id != caption_item->caption_id)
        {
             isFirst = 1;

            previous_x = 0;
            previous_y = 0;
            caption_item->caption_id = tmp->caption_id ;
        }
        {
            float circle_x1 = params->plot_position.x + ((tmp->x - params->min.x) / params->scale.x) * scale_x_num;
            float circle_y1 = params->plot_position.y + params->plot_position.h - ((tmp->y - params->min.y) / params->scale.y) * scale_y_num;

        //    myPrintf("(%f , %f)",circle_x1, circle_y1 );
           // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

           // if (params->dot)
            //    fill_circle(renderer, circle_x1, circle_y1, DOT_RADIUS);

            SDL_SetRenderDrawColor(renderer, (caption_item->caption_color & 0xFF0000) >> 16,
                    (caption_item->caption_color & 0x00FF00) >> 8,
                   caption_item->caption_color & 0x0000FF, 255);

            if (params->dot)
                fill_circle(renderer, circle_x1, circle_y1, DOT_RADIUS - 2);

            if (!isFirst) {
                SDL_RenderDrawLine(renderer, previous_x, previous_y, circle_x1, circle_y1);
            }

            
            previous_x = circle_x1;
            previous_y = circle_y1;

            if (isFirst) {
                isFirst = 0;
            }
        }
        tmp = tmp->nxt;
    }
    

}

/**
 * @brief draw_scale_graduation
 *      draw graduation for the graph
 * @param renderer
 *      SDL renderer object
 * @param params
 *      plot parameters (cf plot_params struct)
 * @param plot
 *      structure containing textures and surfaces
 * @param  params->plot_position.w
 *      plot base width (with proportion to screen width)
 * @param  params->plot_position.h
 *      plot bas heigth (with proportion to screen heigth)
 * @param params->plot_position
 *      SDL rectangle giving position of plot base (x,y) from max x or max y due to stroke width
 * @param font
 *      SDL font used for captions
 * @param font_color
 *      font color to be used
 * @param surface_list
 *      list of surfaces stored to be freed later
 * @param plot_position_x
 *      x position of plot
 * @param  params->plot_position.y
 *      y position of plot
 */
void Plot::draw_scale_graduation(SDL_Renderer * renderer,
        plot_params *params,
        splot *plot,
        SDL_Color font_color) {

    double scale_x_num = params->plot_position.w / ((params->max.x - params->min.x) / params->scale.x);
    double scale_y_num = params->plot_position.h / ((params->max.y - params->min.y) / params->scale.y);

    double init_pos_x = params->plot_position.x;
    double init_pos_y = params->plot_position.y + params->plot_position.h;

    double current_scale = params->min.x;

    double max_point_number_x = (params->max.x / params->scale.x);
    double min_point_number_x = (params->min.x / params->scale.x);

    //int i = 0;

    double regular_caption_text_heigth = 0;
    //double regular_caption_text_width = 0;

    for (int i = min_point_number_x; i < max_point_number_x + 1; i++) {

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        if (params->grid) {
            SDL_RenderDrawLine(renderer, init_pos_x, init_pos_y, init_pos_x, init_pos_y - params->plot_position.h);
        } else {
            SDL_RenderDrawLine(renderer, init_pos_x, init_pos_y, init_pos_x, init_pos_y - GRADUATION_HEIGTH);
            if (!i && min_point_number_x < 0)
                SDL_RenderDrawLine(renderer, init_pos_x, init_pos_y, init_pos_x, init_pos_y - params->plot_position.h);
        }

        char text[10];
        if( params->max.x - params->min.x < 5.0)
        sprintf(text, "%.2f", current_scale);
        else
         sprintf(text, "%d", (int)current_scale);
        

        //        SDL_Surface *caption_text_surface = TTF_RenderText_Blended(font, text, font_color);
        SDL_Rect caption_text;
        //        SDL_Texture * texture_text = SDL_CreateTextureFromSurface(renderer, caption_text_surface);
        //        SDL_QueryTexture(texture_text, NULL, NULL, &caption_text.w, &caption_text.h);
        caption_text.w = FONT_CHARACTER_SIZE * strlen(text);
        caption_text.h = FONT_CHARACTER_SIZE + 2;

        caption_text.x = init_pos_x - caption_text.w / 2;
        caption_text.y = init_pos_y + 5;
        //SDL_RenderCopy(renderer, texture_text, NULL, &caption_text);
        SDL_DrawString(renderer, caption_text.x, caption_text.y, text, 8, &font_color);

        // *surface_list = push_back_surface(*surface_list, caption_text_surface);

        init_pos_x += scale_x_num;
        current_scale += params->scale.x;

        regular_caption_text_heigth = caption_text.h;
    }

    current_scale = params->min.y;

    double max_point_number_y = (params->max.y / params->scale.y);
    double min_point_number_y = (params->min.y / params->scale.y);

    init_pos_x = params->plot_position.x;
    init_pos_y = params->plot_position.y + params->plot_position.h;

    for (int i = min_point_number_y; i < max_point_number_y + 1; i++) {

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        if (params->grid) {
            SDL_RenderDrawLine(renderer, init_pos_x, init_pos_y, init_pos_x + params->plot_position.w, init_pos_y);
        } else {
            SDL_RenderDrawLine(renderer, init_pos_x, init_pos_y, init_pos_x + GRADUATION_HEIGTH, init_pos_y);
            if (!i && min_point_number_y < 0)
                SDL_RenderDrawLine(renderer, init_pos_x, init_pos_y, init_pos_x + params->plot_position.w, init_pos_y);
        }

        char text[10];
        if( params->max.y - params->min.y < 5.0)
        sprintf(text, "%.2f", current_scale);
        else
         sprintf(text, "%d", (int)current_scale);
        

        //        SDL_Surface *caption_text_surface = TTF_RenderText_Blended(font, text, font_color);
        SDL_Rect caption_text;

        //SDL_Texture * texture_text = SDL_CreateTextureFromSurface(renderer, caption_text_surface);
        //SDL_QueryTexture(texture_text, NULL, NULL, &caption_text.w, &caption_text.h);

        caption_text.w = FONT_CHARACTER_SIZE * strlen(text);
        caption_text.h = FONT_CHARACTER_SIZE + 2;
        caption_text.x = init_pos_x - caption_text.w - 10;
        caption_text.y = init_pos_y - caption_text.h / 2;

        //SDL_RenderCopy(renderer, texture_text, NULL, &caption_text);
        SDL_DrawString(renderer, caption_text.x, caption_text.y, text, 8, &font_color);

        //        *surface_list = push_back_surface(*surface_list, caption_text_surface);

        init_pos_y -= scale_y_num;

        current_scale += params->scale.y;

//        regular_caption_text_width = FONT_CHARACTER_SIZE + 2;
    }

    //caption y
    SDL_Rect text_caption_y;
    text_caption_y.w = FONT_CHARACTER_SIZE * strlen(params->caption_text_y);
    text_caption_y.h = FONT_CHARACTER_SIZE + 2;
    text_caption_y.x =  params->plot_position.x/2 - FONT_CHARACTER_SIZE -2;
    text_caption_y.y = params->plot_position.y + params->plot_position.h / 2 + text_caption_y.w / 4;

    SDL_DrawString_Flip(renderer, text_caption_y.x, text_caption_y.y, params->caption_text_y, 8, &font_color);

    //caption x
    SDL_Rect text_caption_x;
    //plot->textureX = SDL_CreateTextureFromSurface(plot->renderer, plot->captionX);
    // SDL_QueryTexture(plot->textureX, NULL, NULL, &text_caption_x.w, &text_caption_x.h);
    text_caption_x.w = FONT_CHARACTER_SIZE * strlen(params->caption_text_x);
    text_caption_x.h = FONT_CHARACTER_SIZE + 2;
    text_caption_x.x = params->screen_width / 2 - text_caption_x.w / 2;
    text_caption_x.y = params->plot_position.y + params->plot_position.h + 5 + 1 * regular_caption_text_heigth;
    //SDL_RenderCopy(plot->renderer, plot->textureX, NULL, &text_caption_x);
    SDL_DrawString(renderer, text_caption_x.x, text_caption_x.y, params->caption_text_x, 8, &font_color);


}

/**
 * @brief wait_for_sdl_event
 *      wait for an event
 */
void Plot::wait_for_sdl_event() {

    SDL_Event event;

    keepRunning = 2;

    // SDL_Thread * thread = SDL_CreateThread(&run, "run", (void *) this);
    while (keepRunning) {

        SDL_PollEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                keepRunning = 0;
                myPrintf(" About to quit window\n");
                break;
            default:
            {
               // myPrintf(" Polling for events\n");

                mainloop(this);
                SDL_Delay(50);
                break;
            }
        };
    }
}

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
void Plot::draw_circle(SDL_Renderer *renderer, int n_cx, int n_cy, int radius) {
    // if the first pixel in the screen is represented by (0,0) (which is in sdl)
    // remember that the beginning of the circle is not in the middle of the pixel
    // but to the left-top from it:

    double error = (double) -radius;
    double x = (double) radius - 0.5;
    double y = (double) 0.5;
    double cx = n_cx - 0.5;
    double cy = n_cy - 0.5;

    while (x >= y) {
        SDL_RenderDrawPoint(renderer, (int) (cx + x), (int) (cy + y));
        SDL_RenderDrawPoint(renderer, (int) (cx + y), (int) (cy + x));

        if (x != 0) {
            SDL_RenderDrawPoint(renderer, (int) (cx - x), (int) (cy + y));
            SDL_RenderDrawPoint(renderer, (int) (cx + y), (int) (cy - x));
        }

        if (y != 0) {
            SDL_RenderDrawPoint(renderer, (int) (cx + x), (int) (cy - y));
            SDL_RenderDrawPoint(renderer, (int) (cx - y), (int) (cy + x));
        }

        if (x != 0 && y != 0) {
            SDL_RenderDrawPoint(renderer, (int) (cx - x), (int) (cy - y));
            SDL_RenderDrawPoint(renderer, (int) (cx - y), (int) (cy - x));
        }

        error += y;
        ++y;
        error += y;

        if (error >= 0) {
            --x;
            error -= x;
            error -= x;
        }
    }
}

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
void Plot::fill_circle(SDL_Renderer *renderer, int cx, int cy, int radius) {
    // Note that there is more to altering the bitrate of this 
    // method than just changing this value.  See how pixels are
    // altered at the following web page for tips:
    //   http://www.libsdl.org/intro.en/usingvideo.html
    static const int BPP = 4;

    // SDL_RenderSetScale( renderer, 3, 3 );

    double r = (double) radius;

    double dy = 0;

    for (dy = 1; dy <= r; dy += 1.0) {
        // This loop is unrolled a bit, only iterating through half of the
        // height of the circle.  The result is used to draw a scan line and
        // its mirror image below it.

        // The following formula has been simplified from our original.  We
        // are using half of the width of the circle because we are provided
        // with a center and we need left/right coordinates.

        double dx = floor(sqrt((2.0 * r * dy) - (dy * dy)));
        int x = cx - dx;

        for (; x <= cx + dx; x++) {
            SDL_RenderDrawPoint(renderer, x, ((int) (cy + r - dy)));
            SDL_RenderDrawPoint(renderer, x, ((int) (cy - r + dy)));
        }
    }

    // SDL_RenderSetScale( renderer, 1, 1);
}