#pragma once
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

#include <OpenGL/glu.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>


namespace syphon
{


struct ServerDesc
{
    ServerDesc(){}
    ServerDesc(const std::string& name, const std::string& app_name)
    : name(name), app_name(app_name) {}
    
    std::string name="none";
    std::string app_name="none";
};


class Server 
{
	public:
        Server();
        ~Server();
        
        void release();
        void init();

        void set_name(const std::string& name);
        std::string get_name() const;

        void publish(GLuint id, GLenum target, GLsizei width, GLsizei height, bool flipped);

        void *syphon;
};

class ServerDirectory;

class Client 
{
	public:
	Client();
	~Client();
    void release();
    void init();

    void set(const ServerDesc& desc);
    bool has_valid_server() const { return server_desc.name != "none"; }
    
    bool bind(int sampler);
    void unbind();
    
    void servers_updated(ServerDirectory* directory);
    
    struct
    {
        int width;
        int height;
        GLuint id;
        GLenum target;
        GLint format;
        GLenum pixel_format;
    } texture;

    void* syphon;
    void* latest_image;
    ServerDesc server_desc;
private:
    int bound_sampler;
};

class ServerDirectory
{
    public:
        ServerDirectory();
        ~ServerDirectory();
        
        void release();
        void init();

        void refresh();
        void register_client(Client* client);
        void remove_client(Client* client);
        bool server_exists(const ServerDesc& desc) const;

        std::vector<Client*> client_list;
        std::vector<ServerDesc> server_list;
        bool initialized;
        
    private:
        void remove_observers();
        void add_observers();
};
}

