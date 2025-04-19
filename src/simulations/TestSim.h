#include "project.h"

class TestSim : public Project
{
public:

    double bx = 0;
    double by = 0;

    void process()
    {
        bx = rand() % 500;
        by = rand() % 500;
    }

    void draw(Canvas* canvas)
    {
        canvas->setFillStyle(255, 0, 0);
        canvas->beginPath();
        canvas->drawCircle(bx, by, 50);
        canvas->fill();
    }
};