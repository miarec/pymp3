# - Find mad
# Find the native mad includes and libraries
#
#  MAD_INCLUDE_DIRS - where to find mad.h, etc.
#  MAD_LIBRARIES    - List of libraries when using mad.
#  MAD_FOUND        - True if mad found.

if (MAD_INCLUDE_DIR)
    # Already in cache, be silent
    set(MAD_FIND_QUIETLY TRUE)
endif ()

find_path (MAD_INCLUDE_DIR mad.h
	HINTS
		${MAD_ROOT}
	)

# MSVC built mad may be named libmad_static.
# The provided project files name the library with the lib prefix.

find_library (MAD_LIBRARY
	NAMES
		libmad
		libmad_static
		libmad-static
	HINTS
		${MAD_ROOT}
	)

# Handle the QUIETLY and REQUIRED arguments and set MAD_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args (mad
	REQUIRED_VARS
		MAD_LIBRARY
		MAD_INCLUDE_DIR
	)

if (MAD_FOUND)
	set (MAD_LIBRARIES ${MAD_LIBRARY})
	set (MAD_INCLUDE_DIRS ${MAD_INCLUDE_DIR})

	if (NOT TARGET mad::mad)
		add_library (mad::mad UNKNOWN IMPORTED)
		set_target_properties (mad::mad PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${MAD_INCLUDE_DIRS}"
			IMPORTED_LOCATION "${MAD_LIBRARY}"
		)
	endif ()
endif ()

mark_as_advanced(MAD_INCLUDE_DIR MAD_LIBRARY)