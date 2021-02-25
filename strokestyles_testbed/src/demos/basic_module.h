#pragma once
#include "colormotor.h"
#include "common.h"

using namespace cm;

class BasicModule : public AppModule
{
public:
    int tool=0; // Current "tool" for geometry editing
    
    ContourMaker ctrMaker;
    ParamList params;

    struct
    {
        float foo = 0;
        bool check = true;
        int count = 0;
    } gui_params;

    BasicModule()
    :
    AppModule("Basic module example")
    {
        // paramters can be saved and added to the UI
        params.addFloat("float", &gui_params.foo, 0.0, 1.)->describe("A floating point paramter with bounds");
        params.addBool("checkbox", &gui_params.check)->describe("A boolean paramter");
        params.addInt("count", &gui_params.count)->describe("An integer paramter");
        
        params.loadXml(this->filename() + ".xml");
        ctrMaker.load(this->filename() + "_ctr.json");
    }
    
    bool init()
    {
        return true;
    }
    
    void exit()
    {
        params.saveXml(this->filename() + ".xml");
        ctrMaker.save(this->filename() + "_ctr.json");
    }
    
    bool gui()
    {
        // UI callback (every frame)
        
        // Using gfx_ui to edit a simple polyline
        ui::begin();
        tool = ui::toolbar("tools", "ab", tool);
        ctrMaker.interact(tool);
        ui::end();
        
        // Can directly call IMGUI here
        if (ImGui::Button("Clear"))
            ctrMaker.clear();
        
        // And automatically create UI for paramters
        imgui(params);         
        return false;
    }
    
    
    void update()
    {
        // Gets called every frame before render
    }
    
    void render()
    {
        // Rendering callback
        float w = appWidth();
        float h = appHeight();
        
        gfx::clear(1,1,1,1);
        
        gfx::setOrtho(w, h);
        gfx::color(1,0,0);
        
        gfx::lineStipple(2);
        gfx::color(1, 0, 0);
        ctrMaker.draw();
        gfx::lineStipple(0);
    }
};




