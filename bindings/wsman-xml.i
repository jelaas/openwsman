/*
 * wsman-xml.i
 * xml structure accessors for openwsman swig bindings
 *
 */

/*
 * class XmlDoc
 *
 */

%extend _WsXmlDoc {
  /* constructor */
  _WsXmlDoc(const char *rootname) {
    return wsman_create_doc(rootname);
  }
  /* destructor */
  ~_WsXmlDoc() {
    ws_xml_destroy_doc( $self );
  }
  %typemap(newfree) char * "free($1);";
#if defined(SWIGRUBY)
  %newobject to_s;
  char *to_s() {
    int size;
    char *buf;
    ws_xml_dump_memory_node_tree( ws_xml_get_doc_root($self), &buf, &size );
    return buf;
  }
#endif
  /* dump doc as string */
  %newobject dump;
  char *dump(const char *encoding="utf-8") {
    int size;
    char *buf;
    ws_xml_dump_memory_enc( $self, &buf, &size, encoding );
    return buf;
  }
  /* dump doc to file */
  void dump_file(FILE *fp) {
    ws_xml_dump_doc( fp, $self );
  }			      
  /* get root node of doc */
  WsXmlNodeH root() {
    return ws_xml_get_doc_root( $self );
  }
  /* get soap envelope */
  WsXmlNodeH envelope() {
    return ws_xml_get_soap_envelope( $self );
  }
  /* get soap header */
  WsXmlNodeH header() {
    return ws_xml_get_soap_header( $self );
  }
  /* get soap body */
  WsXmlNodeH body() {
    return ws_xml_get_soap_body( $self );
  }
  /* get soap element by name */
  WsXmlNodeH element(const char *name) {
    return ws_xml_get_soap_element( $self, name );
  }
  /* get enum context */
  const char *context() {
    return wsmc_get_enum_context( $self );
  }
  %newobject generate_fault;
  WsXmlDocH generate_fault(WsmanStatus *s) {
    return wsman_generate_fault( $self, s->fault_code, s->fault_detail_code, s->fault_msg);
  }
  %newobject create_response_envelope;
  WsXmlDocH create_response_envelope(const char *action = NULL) {
    return wsman_create_response_envelope($self, action);
  }
}


/*
 * class XmlNode
 *
 */

%extend __WsXmlNode {
#if defined(SWIGRUBY)
  %alias text "to_s";
#endif
  %newobject dump;
  /* dump node as XML string */
  char *dump() {
    int size;
    char *buf;
    ws_xml_dump_memory_node_tree( $self, &buf, &size );
    return buf;
  }
  /* dump node to file */
  void dump_file(FILE *fp) {
    ws_xml_dump_node_tree( fp, $self );
  }
  
#if defined(SWIGRUBY)
  %alias equal "==";
  %typemap(out) int equal
    "$result = ($1 != 0) ? Qtrue : Qfalse;";
#endif
#if defined(SWIGPERL)
  int __eq__( WsXmlNodeH n )
#else
  int equal( WsXmlNodeH n )
#endif
  { return $self == n; }	  
  
  /* get text (without xml tags) of node */
  char *text() {
    return ws_xml_get_node_text( $self );
  }
#if defined(SWIGRUBY)
  %rename( "text=" ) set_text( const char *text );
#endif
  void set_text( const char *text ) {
    ws_xml_set_node_text( $self, text );
  }
  /* get doc for node */
  WsXmlDocH doc() {
    return ws_xml_get_node_doc( $self );
  }
  /* get parent for node */
  WsXmlNodeH parent() {
    return ws_xml_get_node_parent( $self );
  }
#if defined(SWIGRUBY)
  %alias child "first";
#endif
  /* get first child of node */
  WsXmlNodeH child() {
    if( ws_xml_get_child_count( $self ) > 0 )
      return ws_xml_get_child($self, 0, NULL, NULL);
    return NULL;
  }
  /* get name for node */
  char *name() {
    return ws_xml_get_node_local_name( $self );
  }
#if defined(SWIGRUBY)
  %rename("name=") set_name( const char *name);
#endif
  /* set name of node */
  void set_name( const char *name ) {
    ws_xml_set_node_name( $self, ws_xml_get_node_name_ns( $self ), name );
  }
  
  /* get namespace for node */
  char *ns() {
    return ws_xml_get_node_name_ns( $self );
  }
#if defined(SWIGRUBY)
  %rename("ns=") set_ns( const char *nsuri );
#endif
  /* set namespace of node */
  void set_ns( const char *ns ) {
    ws_xml_set_ns( $self, ns, ws_xml_get_node_name_ns_prefix($self) );
  }

  /* get prefix of nodes namespace */
  const char *prefix() {
    return ws_xml_get_node_name_ns_prefix($self);
  }

  /* set language */
#if defined(SWIGRUBY)
  %rename("lang=") set_lang(const char *lang);
#endif
  void set_lang(const char *lang) {
    ws_xml_set_node_lang($self, lang);
  }

  /* find node within tree */
  WsXmlNodeH find( const char *ns, const char *name, int recursive = 1) {
    return ws_xml_find_in_tree( $self, ns, name, recursive );
  }
				 
  /* count node children */
  int size() {
    return ws_xml_get_child_count( $self );
  }
  /* add child to node */
  WsXmlNodeH add( const char *ns, const char *name, const char *value = NULL ) {
    return ws_xml_add_child( $self, ns, name, value );
  }

  /* iterate children */

#if defined(SWIGRUBY)
  void each() {
    int i = 0;
    while ( i < ws_xml_get_child_count( $self ) ) {
      rb_yield( SWIG_NewPointerObj((void*) ws_xml_get_child($self, i, NULL, NULL), SWIGTYPE_p___WsXmlNode, 0));
      ++i;
    }
  }
#endif

#if defined(SWIGPYTHON)
  %pythoncode %{
    def __iter__(self):
      r = range(0,self.size())
      while r:
        yield self.get(r.pop(0))
  %}
#endif

#if defined(SWIGRUBY)
  %alias get "[]";
#endif
  /*
   * get child by index
   */
  WsXmlNodeH get(int i) {
    if (i < 0 || i >= ws_xml_get_child_count($self))
      return NULL;
    return ws_xml_get_child($self, i, NULL, NULL);
  }
  
  /*
   * get child by name
   */
  WsXmlNodeH get(const char *name) {
    int i = 0;
    while ( i < ws_xml_get_child_count($self)) {
      WsXmlNodeH child = ws_xml_get_child($self, i, NULL, NULL);
      if (!strcmp(ws_xml_get_node_local_name(child), name))
        return child;
    }
    return NULL;
  }
  
#if 0 /* defined(SWIGRUBY) */
  %rubycode %{
    def method_missing( name, *args )
      if args then
        find(args[0], name) 
      else
        find(nil, name)
      end
    end
  %}
#endif

  /* get node attribute */
  WsXmlAttrH attr(int index = 0) {
    return ws_xml_get_node_attr( $self, index );
  }
  /* count node attribute */
  int attr_count() {
    return ws_xml_get_node_attr_count( $self );
  }
  /* find node attribute by name */
  WsXmlAttrH attr_find( const char *ns, const char *name ) {
    return ws_xml_find_node_attr( $self, ns, name );
  }
  /* add attribute to node */
  WsXmlAttrH attr_add( const char *ns, const char *name, const char *value ) {
    return ws_xml_add_node_attr( $self, ns, name, value );
  }

  epr_t *epr( const char *ns, const char *epr_node_name, int embedded) {
    return epr_deserialize($self, ns, epr_node_name, embedded);
  }  


#if defined(SWIGRUBY)
  /* enumerate attributes */
  void each_attr() {
    int i = 0;
    while ( i < ws_xml_get_node_attr_count( $self ) ) {
      rb_yield( SWIG_NewPointerObj((void*) ws_xml_get_node_attr($self, i), SWIGTYPE_p___WsXmlAttr, 0));
      ++i;
    }
  }
#endif
}




/*
 * class XmlAttr
 *
 */


%extend __WsXmlAttr {
#if defined(SWIGRUBY)
  %alias value "to_s";
#endif
  /* get name for attr */
  char *name() {
    return ws_xml_get_attr_name( $self );
  }
  /* get namespace for attr */
  char *ns() {
    return ws_xml_get_attr_ns( $self );
  }
  /* get value for attr */
  char *value() {
    return ws_xml_get_attr_value( $self );
  }
  /* remove note attribute */
  void remove() {
    ws_xml_remove_node_attr( $self );
  }
}

