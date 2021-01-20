#include "syphon_wrapper.h"
#import <Syphon.h>
#import "SyphonNameboundClient.h"

namespace syphon
{

Server::Server()
{
	syphon = nil;
}

Server::~Server()
{
    release();
}

void Server::release()
{
    if (syphon)
    {
        NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
        [(SyphonServer *)syphon stop];
        [(SyphonServer *)syphon release];
        
        [pool drain];

        syphon = 0;
    }
}

void Server::set_name(const std::string& name)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
	NSString *title = [NSString stringWithCString:name.c_str()
										 encoding:[NSString defaultCStringEncoding]];
	
	if (!syphon)
	{
		syphon = [[SyphonServer alloc] initWithName:title context:CGLGetCurrentContext() options:nil];
	}
	else
	{
		[(SyphonServer *)syphon setName:title];
	}
    
    [pool drain];
}

std::string Server::get_name() const
{
	std::string name;
	if (syphon)
	{
		NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
        
		name = [[(SyphonServer *)syphon name] cStringUsingEncoding:[NSString defaultCStringEncoding]];
		
		[pool drain];
	}
	else
	{
		name = "Untitled";
	}
	return name;
}

void Server::publish(GLuint id, GLenum target, GLsizei width, GLsizei height, bool flipped)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    if (!syphon)
    {
        syphon = [[SyphonServer alloc] initWithName:@"Untitled" context:CGLGetCurrentContext() options:nil];
    }
    
    [(SyphonServer *)syphon publishFrameTexture:id textureTarget:target imageRegion:NSMakeRect(0, 0, width, height) textureDimensions:NSMakeSize(width, height) flipped:!flipped];
    [pool drain];   
}

Client::Client() :
syphon(nil)
{
    texture.width = 0; 
    texture.height = 0;
    texture.id = 0;
    // These are constant
    texture.target = GL_TEXTURE_RECTANGLE_ARB;
    texture.format = GL_RGBA;
    texture.pixel_format = GL_UNSIGNED_BYTE;
}

Client::~Client()
{
    release();
}

void Client::release()
{
    if (syphon)
    {
        NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

        [(SyphonNameboundClient*)syphon release];
        syphon = nil;
        
        [pool drain];

        syphon = nil;
    }
}


void Client::init()
{
    // Need pool
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];        
	syphon = [[SyphonNameboundClient alloc] initWithContext:CGLGetCurrentContext()];
    [pool drain];
}


void Client::set(const ServerDesc& server_desc)
{
    if(syphon)
    {
        this->server_desc = server_desc;
        NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
        
        NSString *nsAppName = [NSString stringWithCString:server_desc.app_name.c_str() encoding:[NSString defaultCStringEncoding]];
        NSString *nsServerName = [NSString stringWithCString:server_desc.name.c_str() encoding:[NSString defaultCStringEncoding]];
        
        [(SyphonNameboundClient*)syphon setAppName:nsAppName];
        [(SyphonNameboundClient*)syphon setName:nsServerName];
        
        [pool drain];
    }
}

bool Client::bind(int sampler)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    GLint id = -1;
    
    if (!has_valid_server())
        return false;
        
    if(syphon)
    {
     	[(SyphonNameboundClient*)syphon lockClient];
        SyphonClient *client = [(SyphonNameboundClient*)syphon client];
        
        latest_image = [client newFrameImage];
		NSSize size = [(SyphonImage*)latest_image textureSize];
        id = texture.id = [(SyphonImage*)latest_image textureName];
        texture.width = size.width;
        texture.height = size.height;
        
        bound_sampler = sampler;
        glActiveTexture( GL_TEXTURE0+sampler );
        glEnable(texture.target);
        glBindTexture(texture.target, id);
        
    }
    else
    {
        std::cout << "Client is not seutp, cannnot bind\n";
    }

    [pool drain];
    return id != -1;
}

void Client::unbind()
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    if(syphon)
    {
        [(SyphonNameboundClient*)syphon unlockClient];
        [(SyphonImage*)latest_image release];
        latest_image = nil;
        
        glActiveTexture( GL_TEXTURE0 + bound_sampler );
        glDisable(texture.target);
        glBindTexture(texture.target, 0);
    }
    else
    {
		std::cout << "Client is not setup, or is not properly connected to server.  Cannot unbind.\n";
    }
    
    [pool drain];
}

void Client::servers_updated(ServerDirectory* directory)
{
    if (server_desc.name=="none")
        return;
    
    if (!directory->server_exists(server_desc))
        server_desc.name = server_desc.app_name = "none";
}

static std::string CFStringRefToString(CFStringRef src)
{
    std::string dest;

    const char *cstr = CFStringGetCStringPtr(src, CFStringGetSystemEncoding());
    if (!cstr)
    {
        CFIndex strLen = CFStringGetMaximumSizeForEncoding(CFStringGetLength(src) + 1,
                                                           CFStringGetSystemEncoding());
        char *allocStr = (char*)malloc(strLen);
        
        if(!allocStr)
            return "";
        
        if(!CFStringGetCString(src, allocStr, strLen, CFStringGetSystemEncoding()))
        {
            free((void*)allocStr);
            return "";
        }
        
        dest = allocStr;
        free((void*)allocStr);
        
        return dest;
    }
    
    dest = cstr;
    return dest;
}

static void notificationHandler(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo) 
{
    ServerDirectory * directory = (ServerDirectory*)observer;

    if((NSString*)name == SyphonServerAnnounceNotification)
    {
        directory->refresh();
    }
    else if((NSString*)name == SyphonServerUpdateNotification)
    {
        //serverUpdated();
    } 
    else if((NSString*)name == SyphonServerRetireNotification)
    {
        directory->refresh();
    }
}

ServerDirectory::ServerDirectory()
{
    initialized = false;
}

ServerDirectory::~ServerDirectory()
{
    release();
}

void ServerDirectory::release()
{
    server_list.clear();
    refresh();
    client_list.clear();

    if (initialized)
    {   
        remove_observers();
    }
}

void ServerDirectory::init()
{
    if (!initialized)
    {
        add_observers();
        refresh();
    }
}

void ServerDirectory::refresh()
{
    server_list.clear();
    
    for(NSDictionary* serverDescription in [[SyphonServerDirectory sharedDirectory] servers])
    {
        NSString* name = [serverDescription objectForKey:SyphonServerDescriptionNameKey];
        NSString* appName = [serverDescription objectForKey:SyphonServerDescriptionAppNameKey];
        server_list.push_back(ServerDesc(std::string([name UTF8String]),std::string([appName UTF8String])));
    }
    
    // Check if any client has been deabilitated
    for (int i = 0; i < client_list.size(); i++)
    {
        client_list[i]->servers_updated(this);
    }
}

void ServerDirectory::register_client(Client* client)
{
    auto it = std::find(client_list.begin(), client_list.end(), client);
    if (it == client_list.end())
        client_list.push_back(client);
}

void ServerDirectory::remove_client(Client* client)
{
    auto it = std::find(client_list.begin(), client_list.end(), client);
    if (it != client_list.end())
        client_list.erase(it);
}

bool ServerDirectory::server_exists(const ServerDesc& server) const
{
    for(auto& s: server_list)
    {
        if(s.name == server.name && s.app_name == server.app_name)
            return true;
    }
    
    return false;
}

void ServerDirectory::add_observers()
{
    CFNotificationCenterAddObserver
    (
     CFNotificationCenterGetLocalCenter(),
     this,
     (CFNotificationCallback)&notificationHandler,
     (CFStringRef)SyphonServerAnnounceNotification,
     NULL,
     CFNotificationSuspensionBehaviorDeliverImmediately
     );
    
    CFNotificationCenterAddObserver
    (
     CFNotificationCenterGetLocalCenter(),
     this,
     (CFNotificationCallback)&notificationHandler,
     (CFStringRef)SyphonServerUpdateNotification,
     NULL,
     CFNotificationSuspensionBehaviorDeliverImmediately
     );
    
    CFNotificationCenterAddObserver
    (
     CFNotificationCenterGetLocalCenter(),
     this,
     (CFNotificationCallback)&notificationHandler,
     (CFStringRef)SyphonServerRetireNotification,
     NULL,
     CFNotificationSuspensionBehaviorDeliverImmediately
     );
}

void ServerDirectory::remove_observers()
{
    CFNotificationCenterRemoveEveryObserver(CFNotificationCenterGetLocalCenter(), this);
}


}
