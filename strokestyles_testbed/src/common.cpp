#include "cm_imgui_app.h"
#include "common.h"
#include <boost/filesystem.hpp>

using namespace arma; 

namespace ag
{
void PaletteManager::draw( const arma::vec& p, double swatch_size )
    {
        using namespace cm;
        double x = p[0];
        double y = p[1];
        for (int i = 0; i < palette.size(); i++)
        {
            gfx::color(palette.color(i));
            gfx::fillRect(x, y, swatch_size, swatch_size);
            x += swatch_size;
        }
    }

namespace fs = boost::filesystem;
std::string relative_path(const std::string& full_path)
{
    //https://stackoverflow.com/questions/10167382/boostfilesystem-get-relative-path
    
    fs::path parentPath(cm::getCurrentDirectory());
    fs::path childPath(full_path);
    fs::path relativePath = fs::relative(childPath, parentPath);
    if (relativePath.empty())
    {
        std::cout << "Could not make relative path for " << full_path << std::endl;
        return full_path;
    }
    return relativePath.string();
}

void create_directory(const std::string& path)
{
    fs::create_directory(path);
}


}

