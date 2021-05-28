# DO NOT EDIT
# This makefile makes sure all linkable targets are
# up-to-date with anything they link to
default:
	echo "Do not invoke directly"

# Rules to remove targets that are older than anything to which they
# link.  This forces Xcode to relink the targets from scratch.  It
# does not seem to check these dependencies itself.
PostBuild.strokestyles_testbed.Debug:
/Users/colormotor/develop/autograff/strokestyles_workspace/strokestyles_interactive_demos/strokestyles_testbed/bin/Debug/strokestyles_testbed:
	/bin/rm -f /Users/colormotor/develop/autograff/strokestyles_workspace/strokestyles_interactive_demos/strokestyles_testbed/bin/Debug/strokestyles_testbed


PostBuild.strokestyles_testbed.Release:
/Users/colormotor/develop/autograff/strokestyles_workspace/strokestyles_interactive_demos/strokestyles_testbed/bin/Release/strokestyles_testbed:
	/bin/rm -f /Users/colormotor/develop/autograff/strokestyles_workspace/strokestyles_interactive_demos/strokestyles_testbed/bin/Release/strokestyles_testbed


PostBuild.strokestyles_testbed.MinSizeRel:
/Users/colormotor/develop/autograff/strokestyles_workspace/strokestyles_interactive_demos/strokestyles_testbed/bin/MinSizeRel/strokestyles_testbed:
	/bin/rm -f /Users/colormotor/develop/autograff/strokestyles_workspace/strokestyles_interactive_demos/strokestyles_testbed/bin/MinSizeRel/strokestyles_testbed


PostBuild.strokestyles_testbed.RelWithDebInfo:
/Users/colormotor/develop/autograff/strokestyles_workspace/strokestyles_interactive_demos/strokestyles_testbed/bin/RelWithDebInfo/strokestyles_testbed:
	/bin/rm -f /Users/colormotor/develop/autograff/strokestyles_workspace/strokestyles_interactive_demos/strokestyles_testbed/bin/RelWithDebInfo/strokestyles_testbed




# For each target create a dummy ruleso the target does not have to exist
