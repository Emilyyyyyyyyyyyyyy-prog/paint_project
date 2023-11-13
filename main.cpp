#include "gtkmm.h"
#include "cairomm/context.h"
#include <iostream>

struct Color {
    double r, g, b, a;

    Color(double r, double g, double b, double a = 1) : r(r), g(g), b(b), a(a) {}
};

class Canvas : public Gtk::DrawingArea {
public:
    static const int DEFAULT = 1 << 0;
    static const int DRAWING = 1 << 1;
    static const int BRUSH = 1 << 2;
    static const int LINE = 1 << 3;
private:
    int state;
    int width;
    Color color;
    Cairo::RefPtr<Cairo::Surface> buffer;
    Cairo::RefPtr<Cairo::Surface> temp_buffer;
    double buffer_width, buffer_height;
    double start_x, start_y;
    bool need_fix_temp_buffer;

    Gtk::ColorChooserDialog *color_chooser_dialog;
//    Gtk::ColorChooserDialog *width_chooser_dialog;
public:
    Canvas() : state(DEFAULT | BRUSH), color(0, 0, 0), width(5), buffer_width(400), buffer_height(400),
               need_fix_temp_buffer(false) {
        this->signal_draw().connect(sigc::mem_fun(*this, &Canvas::on_draw));
        this->set_vexpand(true);
        this->add_events(Gdk::BUTTON1_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK);
        this->signal_button_press_event().connect(sigc::mem_fun(*this, &Canvas::on_mouse_press));
        this->signal_motion_notify_event().connect(sigc::mem_fun(*this, &Canvas::on_mouse_move));
        this->signal_button_release_event().connect(sigc::mem_fun(*this, &Canvas::on_mouse_release));
        this->buffer = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, buffer_width, buffer_width);
        this->temp_buffer = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, buffer_width, buffer_width);
        auto context = Cairo::Context::create(this->buffer);
        context->set_source_rgb(1, 1, 1);
        context->rectangle(0, 0, buffer_width, buffer_height);
        context->fill();
        context->stroke();

        this->color_chooser_dialog = new Gtk::ColorChooserDialog;
        this->color_chooser_dialog->set_modal(true);
        this->color_chooser_dialog->signal_response().connect(sigc::mem_fun(*this, &Canvas::choose_color_response));
    }

    void choose_color_response(int response_id) {
        if (response_id == Gtk::RESPONSE_OK) {
            auto res = this->color_chooser_dialog->get_rgba();
            this->color = {res.get_red(), res.get_green(), res.get_blue(), res.get_alpha()};
        }
        this->color_chooser_dialog->hide();
    }

    void choose_color() {
        this->color_chooser_dialog->run();
    }

//    void choose_width(){
//        this->width_chooser_dialog->run();
//    }

//    void choose_width_response(int response_id) {
//        if (response_id == Gtk::RESPONSE_OK) {
//            auto res = this->width_chooser_dialog
//        }
//    }

    bool on_mouse_press(GdkEventButton *event) {
        this->state &= ~DEFAULT;
        this->state |= DRAWING;
        auto context = this->get_context(temp_buffer, true);
        this->start_x = event->x;
        this->start_y = event->y;
        this->drawing(event->x, event->y);
        return true;
    }

    bool on_mouse_move(GdkEventMotion *event) {
        this->drawing(event->x, event->y);
        return true;
    }

    bool on_mouse_release(GdkEventButton *event) {
        this->state &= ~DRAWING;
        this->state |= DEFAULT;
        this->drawing(event->x, event->y);
        return true;
    }

    Cairo::RefPtr<Cairo::Context> get_context(Cairo::RefPtr<Cairo::Surface> &surface, bool need_clear = false) {
        auto context = Cairo::Context::create(surface);
        if (need_clear) {
            context->set_source_rgba(0, 0, 0, 0);
            context->set_operator(Cairo::OPERATOR_CLEAR);
            context->rectangle(0, 0, buffer_width, buffer_height);
            context->paint_with_alpha(1);
            context->stroke();
        }
        context->set_source_rgba(this->color.r, this->color.g, this->color.b, this->color.a);
        context->set_operator(Cairo::OPERATOR_OVER);
        return context;
    }

    void drawing(double x, double y) {
        if (this->state & DRAWING) {
            this->need_fix_temp_buffer = true;
            if (this->state & BRUSH) {
                auto context = this->get_context(temp_buffer);
                context->arc(x, y, 5, 0, M_PI * 2);
                context->fill();
                context->stroke();
            }
            if (this->state & LINE) {
                auto context = this->get_context(temp_buffer, true);
                context->move_to(this->start_x, this->start_y);
                context->line_to(x, y);
                context->stroke();
            }
            this->queue_draw();
        } else if (this->need_fix_temp_buffer) {
            this->need_fix_temp_buffer = false;
            auto context = this->get_context(buffer);
            context->set_source(this->temp_buffer, 0, 0);
            context->paint();
            this->queue_draw();
        }
    }

    bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override {
        cr->set_source(this->buffer, 0, 0);
        cr->paint();
        if (this->state & DRAWING) {
            cr->set_source(this->temp_buffer, 0, 0);
            cr->paint();
        }
        return true;
    }

    void change_tool(int tool) {
        this->state = Canvas::DEFAULT | tool;
    }

};

int main(int argc, char **argv) {
    auto app = Gtk::Application::create(argc, argv);
    auto ui = Gtk::Builder::create_from_file("design.glade");

    Gtk::ApplicationWindow *window;
    ui->get_widget("window", window);

    Canvas *canvas = new Canvas;
    Gtk::Box *box;
    ui->get_widget("box", box);
    box->add(*canvas);
    canvas->show();

    Gtk::ToolButton *tool_color_choose;
    ui->get_widget("tool_color_choose", tool_color_choose);
    tool_color_choose->signal_clicked().connect(sigc::mem_fun(*canvas, &Canvas::choose_color));

    Gtk::ToolButton *tool_brush;
    ui->get_widget("tool_brush", tool_brush);
    tool_brush->signal_clicked().connect(sigc::bind(sigc::mem_fun(*canvas, &Canvas::change_tool), Canvas::BRUSH));

    Gtk::ToolButton *tool_line;
    ui->get_widget("tool_line", tool_line);
    tool_line->signal_clicked().connect(sigc::bind(sigc::mem_fun(*canvas, &Canvas::change_tool), Canvas::LINE));

    return app->run(*window);
}