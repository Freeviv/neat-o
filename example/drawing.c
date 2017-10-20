#include <assert.h>
#include <math.h>

#include <gtk/gtk.h>
#include <nn.h>
#include <neat.h>

static cairo_surface_t *surface = NULL;
static neat_t neat = NULL;

static float xor_inputs[4][2] = {
	{0.0f, 0.0f},
	{0.0f, 1.0f},
	{1.0f, 0.0f},
	{1.0f, 1.0f}
};
static float xor_outputs[4] = {0.0f, 1.0f, 1.0f, 0.0f};

static struct neat_config config = {
	.network_inputs = 2,
	.network_outputs = 1,
	.network_hidden_nodes = 4,

	.population_size = 20,

	.species_crossover_probability = 0.2,
	.interspecies_crossover_probability = 0.05,
	.mutate_species_crossover_probability = 0.25,

	.genome_add_neuron_mutation_probability = 0.5,
	.genome_add_link_mutation_probability = 0.1,

	.genome_minimum_ticks_alive = 100,
	.genome_compatibility_treshold = 0.2
};

static void setup_neat()
{
	neat = neat_create(config);
	assert(neat);
}

static void draw_neuron_circle(cairo_t *cr,
			       guint x,
			       guint y,
			       int radius,
			       float value,
			       bool is_bias)
{
	cairo_save(cr);
	if(is_bias){
		cairo_arc(cr, x, y, radius / 2.0, 0, 2 * G_PI);
	}else{
		cairo_arc(cr, x, y, radius, 0, 2 * G_PI);
	}
	cairo_set_source_rgb(cr, 1.0, (value + 1.0) / 2.0, 1.0);
	cairo_fill_preserve(cr);
	cairo_restore(cr);
	cairo_stroke(cr);
}

static void draw_neat_network(cairo_t *cr,
			      GtkStyleContext *context,
			      size_t network,
			      guint x,
			      guint y,
			      guint width,
			      guint height)
{
	int radius = width / 20, xoffset = width / 10, yoffset = height / 20;

	const struct nn_ffnet *n = neat_get_network(neat, network);

	float *neuron = n->output;

	x += radius + yoffset;
	y += radius + yoffset;
	guint starty = y;

	draw_neuron_circle(cr, x, y, radius, n->bias, true);
	y += radius * 2 + yoffset;
	for(size_t i = 0; i < n->ninputs; i++){
		draw_neuron_circle(cr, x, y, radius, *neuron++, false);
		y += radius * 2 + yoffset;
	}

	x += radius * 2 + xoffset;
	y = starty;

	for(size_t i = 0; i < n->nhidden_layers; i++){
		draw_neuron_circle(cr, x, y, radius, n->bias, true);
		y += radius * 2 + yoffset;
		for(size_t j = 0; j < n->nhiddens; j++){
			draw_neuron_circle(cr, x, y, radius, *neuron++, false);
			y += radius * 2 + yoffset;
		}
		x += radius * 2 + xoffset;
		y = starty;
	}

	for(size_t i = 0; i < n->noutputs; i++){
		y += radius * 2 + yoffset;
		draw_neuron_circle(cr, x, y, radius, *neuron++, false);
	}
}

static gboolean tick(gpointer data)
{
	for(int i = 0; i < config.population_size; i++){
		float error = 0.0f;
		for(int j = 0; j < 4; j++){
			const float *results = neat_run(neat, i, xor_inputs[j]);

			error += fabs(results[0] - xor_outputs[j]);
		}

		if(error < 0.1){
			return FALSE;
		}

		float fitness = 4.0 - error;
		neat_set_fitness(neat, i, fitness * fitness);

		neat_increase_time_alive(neat, i);
	}

	neat_epoch(neat);

	gtk_widget_queue_draw(GTK_WIDGET(data));

	return TRUE;
}

static void clear()
{
	cairo_t *cr = cairo_create(surface);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	cairo_destroy(cr);
}

static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	cairo_set_line_width(cr, 2.0);

	GtkStyleContext *context = gtk_widget_get_style_context(widget);
	guint width = gtk_widget_get_allocated_width(widget);
	guint height = gtk_widget_get_allocated_height(widget);
	gtk_render_background(context, cr, 0, 0, width, height);

	GdkRGBA color;
	gtk_style_context_get_color(context,
				    gtk_style_context_get_state(context),
				    &color);
	gdk_cairo_set_source_rgba(cr, &color);

	for(size_t y = 0; y < 4; y++){
		for(size_t x = 0; x < 5; x++){
			draw_neat_network(cr,
					  context,
					  x + y * 5,
					  x * (width / 5),
					  y * (height / 4),
					  width / 5,
					  height / 4);
		}
	}

	return FALSE;
}

static gboolean configure(GtkWidget *widget,
			  GdkEventConfigure *event,
			  gpointer data)
{
	if (surface){
		cairo_surface_destroy (surface);
	}

	GdkWindow *window = gtk_widget_get_window(widget);
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);
	surface = gdk_window_create_similar_surface(window,
						    CAIRO_CONTENT_COLOR,
						    width,
						    height);

	clear();

	return TRUE;
}

static void close_window()
{
	if(surface){
		cairo_surface_destroy(surface);
	}
}

static void activate(GtkApplication *app, gpointer user_data)
{
	GtkWidget *window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "neat-o: drawing");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);

	gtk_container_set_border_width(GTK_CONTAINER (window), 8);

	GtkWidget *frame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(window), frame);

	GtkWidget *drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawing_area, 300, 300);
	gtk_container_add(GTK_CONTAINER(frame), drawing_area);
	g_signal_connect(drawing_area, "draw", G_CALLBACK(draw), NULL);
	g_signal_connect(drawing_area,
			 "configure-event",
			 G_CALLBACK(configure),
			 NULL);

	g_timeout_add(50, tick, drawing_area);

	gtk_widget_show_all(window);

	setup_neat();
}

int main(int argc, char *argv[])
{	
	GtkApplication *app = gtk_application_new("org.tversteeg.neatc",
						  G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

	int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
