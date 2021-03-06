/*
 *  Copyright (C) 2002-2005  Mattia Dongili <malattia@linux.it>
 *                           George Staikos <staikos@0wned.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fnmatch.h>
#include <stdio.h>
#include <string.h>
#include "ppd.h"
#include "ppd_log.h"
#include "ppd_plugin_utils.h"

static int ppd_plugin_filter(const struct dirent *d) {
	return fnmatch("plugin_*.so", d->d_name, 0) == 0;
}

/*
 * Try to discover pugins
 */
void discover_plugins(struct LIST *plugins) {
	int n = 0;
	struct plugin_obj o_plugin;
	struct NODE *n_plugin;
	struct dirent **namelist;

	/* plugin names */
	n = scandir(PPD_LIBDIR, &namelist, ppd_plugin_filter, NULL);
	if (n > 0) {
		while (n--) {
			o_plugin.library = NULL;
			o_plugin.plugin = NULL;
			o_plugin.used = 0;
//			o_plugin.configured = 0;

			sscanf(namelist[n]->d_name, "plugin_%[^.].so", o_plugin.name);
			o_plugin.name[MAX_STRING_LEN-1] = '\0';

			n_plugin = node_new(&o_plugin, sizeof(struct plugin_obj));
			list_append(plugins, n_plugin);
			clog(LOG_INFO, "found plugin: %s\n", o_plugin.name);

			free(namelist[n]);
		}
		free(namelist);

	} else if (n < 0) {
		clog(LOG_ERR, "error reading %s: %s\n",
				PPD_LIBDIR, strerror(errno));

	} else {
		clog(LOG_WARNING, "no plugins found in %s\n", PPD_LIBDIR);
	}
}

/*
 *  Load plugins from a list of plugin_obj's. Also cleanup the
 *  list if a plugin fails to load
 */
void load_plugin_list(struct LIST *plugins) {
	struct plugin_obj *o_plugin = NULL;
	struct NODE *n = NULL;

	n = plugins->first;
	while (n != NULL) {
		o_plugin = (struct plugin_obj*)n->content;
		/* take care!! if statement badly indented!! */
		if (load_plugin(o_plugin) == 0 &&
				get_plugin_object(o_plugin) == 0 &&
				initialize_plugin(o_plugin) == 0) {
			clog(LOG_INFO, "plugin loaded: %s\n", o_plugin->plugin->plugin_name);
			n = n->next;

		} else {
			clog(LOG_INFO, "plugin failed to load: %s\n", o_plugin->name);
			/* remove the list item and assing n the next node (returned from list_remove_node) */
			clog(LOG_NOTICE, "discarded plugin %s\n", o_plugin->name);
			n = list_remove_node(plugins, n);
		} /* end else */
	} /* end while */
}

/* Validate plugins after parsing the configuration, an unused
 * plugin is unloaded and removed from the list.
 *
 * Returns the number of remaining plugins.
 */
int validate_plugins(struct LIST *plugins) {
	struct plugin_obj *o_plugin = NULL;
	struct NODE *n = NULL;
	int used_plugins = 0;

	n = plugins->first;
	while (n != NULL) {
		o_plugin = (struct plugin_obj*)n->content;
		if (o_plugin->used > 1) {
			used_plugins++;
			n = n->next;
		} else {
			clog(LOG_INFO, "%s plugin is unused.\n", o_plugin->name);
			finalize_plugin((struct plugin_obj*)n->content);
			close_plugin((struct plugin_obj*)n->content);
			n = list_remove_node(plugins, n);
		}
	}
	return used_plugins;
}

/*  int load_plugin(struct plugin_obj *cp)
 *  Open shared libraries
 */
int load_plugin(struct plugin_obj *cp) {
	char libname[512];

	snprintf(libname, 512, PPD_LIBDIR"plugin_%s.so", cp->name);

	clog(LOG_INFO, "Loading \"%s\" for plugin \"%s\".\n", libname, cp->name);
	cp->library = dlopen(libname, RTLD_LAZY);
	if (!cp->library) {
		clog(LOG_ERR, "%s\n", dlerror());
		return -1;
	}

	return 0;
}

/*  void close_plugin(struct plugin_obj *cp)
 *  Close shared libraries
 */
void close_plugin(struct plugin_obj *cp) {
	/* close library */
	if (dlclose(cp->library) != 0) {
		clog(LOG_ERR, "Error unloading plugin %s: %s\n", cp->name, dlerror());
		return;
	}
	clog(LOG_INFO, "%s plugin closed.\n", cp->name);
}

/*  int get_plugin_object(struct plugin_obj *cp)
 *  Calls the create_plugin routine.
 */
int get_plugin_object(struct plugin_obj *cp) {

	/* pointer to an error message, if any */
	const char* error;
	/* plugin ptr */
	PowerPolicyDaemonPlugin *(*create)(void);

	clog(LOG_INFO, "Getting plugin object for \"%s\".\n", cp->name);
	/* create plugin */
	create = (PowerPolicyDaemonPlugin * (*) (void))dlsym(cp->library, "create_plugin");
	error = dlerror();
	if (error) {
		clog(LOG_ERR, "get_plugin_object(): %s\n", error);
		return -1;
	}
	cp->plugin = create();

	return 0;
}

/*  int initialize_plugin(struct plugin_obj *cp)
 *  Call plugin_init()
 */
int initialize_plugin(struct plugin_obj *cp) {
	int ret = 0;
	clog(LOG_INFO, "Initializing plugin \"%s-%s\".\n",
			cp->name, cp->plugin->plugin_name);
	/* call init function */
	if (cp->plugin->plugin_init != NULL) {
		ret = cp->plugin->plugin_init();
	}
	return ret;
}

/*  int finalize_plugin(struct plugin_obj *cp)
 *  Call plugin_exit()
 */
int finalize_plugin(struct plugin_obj *cp) {
	if (cp != NULL && cp->plugin->plugin_exit != NULL) {
		clog(LOG_INFO, "Finalizing plugin \"%s-%s\".\n",
				cp->name, cp->plugin->plugin_name);
		/* call exit function */
		cp->plugin->plugin_exit();
		return -1;
	}
	return 0;
}

void plugins_post_conf(struct LIST *plugins) {
	struct NODE *node = NULL;
	struct plugin_obj *plugin = NULL;
	/* plugin POST CONFIGURATION */
	node = plugins->first;
	while (node) {
		plugin = (struct plugin_obj *) node->content;
		/* try to post-configure the plugin */
		if (plugin->plugin->plugin_post_conf != NULL
				&& plugin->plugin->plugin_post_conf() != 0) {
			clog(LOG_NOTICE, "Unable to configure plugin %s, removing\n",
					plugin->plugin->plugin_name);

			/* the next call is currently useless due to the fact that
			 * plugins are post-conf'ed before any rule/profile is read
			 * not now
			 */
			/*
			deconfigure_plugin(config, plugin);
			*/
			finalize_plugin(plugin);
			close_plugin(plugin);
			node = list_remove_node(plugins, node);
		} else {
			node = node->next;
		}
	}
}

/*
 * Looks for a plugin handling the key keyword, calls its parse function
 * and assigns the obj as returned by the plugin. Returns the struct
 * ppd_keyword handling the keyword or NULL if no plugin handles the
 * keyword or if an error occurs parsing the value.
 * NOTE: the value of obj and plugin is significant only if the function
 * returns non-NULL.
 */
PowerPolicyDaemonKeyword *plugin_handle_keyword(struct LIST *plugins,
		const char *key, const char *value, void **obj,
		PowerPolicyDaemonPlugin **plugin) {
	PowerPolicyDaemonKeyword *ckw = NULL;
	struct plugin_obj *o_plug = NULL;

	/* foreach plugin */
	LIST_FOREACH_NODE(node, plugins) {
		o_plug = (struct plugin_obj*)node->content;
		if (o_plug == NULL || o_plug->plugin == NULL ||
				o_plug->plugin->keywords == NULL ||
				!o_plug->used)
			continue;

		/* foreach keyword */
		for(ckw = o_plug->plugin->keywords; ckw->word != NULL; ckw++) {

			/* if keyword corresponds
			 */
			if (strncmp(ckw->word, key, MAX_STRING_LEN) != 0)
				continue;

			clog(LOG_DEBUG, "Plugin %s handles keyword %s (value=%s)\n",
					o_plug->plugin->plugin_name, key, value);

			if (ckw->parse(value, obj) != 0) {
				clog(LOG_ERR, "%s is unable to parse this value \"%s\". Discarded\n",
						o_plug->plugin->plugin_name, value);
				return NULL;
			}
			/* increase plugin use count */
			o_plug->used++;
			*plugin = o_plug->plugin;
			return ckw;
		}
	}
	clog(LOG_NOTICE, "unhandled keyword \"%s\". Discarded\n", key);
	return NULL;
}

/*
 * Tries to free the object using the plugin provided free function.
 * Falls back to the libc function.
 */
void free_keyword_object(PowerPolicyDaemonKeyword *k, void *obj) {
	if(!obj)
		return;

	if (k->free != NULL)
		k->free(obj);
	else
		free(obj);
}
