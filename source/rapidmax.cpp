/**
	rapidmax - a max object which contains machine learning models exported from rapidmax

*/

#include "ext.h"							
#include "ext_obex.h"

#include "ext_dictobj.h"

#include "regression.h"
#include "classification.h"

typedef struct _rapidmax
{
	t_object ob;
    void *m_outlet1, *m_outlet2;
    std::vector<trainingExample> trainingSet;
    std::string modeltype;
    modelSet *model;
    bool trained = false;
    
} t_rapidmax;

void *rapidmax_new(t_symbol *s, long argc, t_atom *argv);
void rapidmax_free(t_rapidmax *x);
void rapidmax_assist(t_rapidmax *x, void *b, long m, long a, char *s);
static t_class *rapidmax_class;

void rapidmax_train(t_rapidmax *x, t_symbol *s, long argc, t_atom *argv);
void rapidmax_regress(t_rapidmax *x, t_symbol *s, long argc, t_atom *argv);
void rapidmax_classify(t_rapidmax *x, t_symbol *s, long argc, t_atom *argv);
int  rapidmax_fill_training_example(t_rapidmax *x, std::vector<double> &v, long argc, t_atom *argv);

void free_dict(t_dictionary *d, long numkeys, t_symbol **keys);
void rapidmax_initialize(t_rapidmax *x);

void rapidmax_read(t_rapidmax *x, t_symbol *s);
void rapidmax_doread(t_rapidmax *x, t_symbol *s);

void rapidmax_write(t_rapidmax *x, t_symbol *s);
void rapidmax_dowrite(t_rapidmax *x, t_symbol *s);

void rapidmax_process_int(t_rapidmax *x, long l);
void rapidmax_process_float(t_rapidmax *x, double f);
void rapidmax_process_list(t_rapidmax *x, t_symbol *s, long argc, t_atom *argv);
void rapidmax_process(t_rapidmax *x, long argc, float *argv);


void ext_main(void *r)
{
	t_class *c;

	c = class_new("rapidmax", (method)rapidmax_new, (method)rapidmax_free, (long)sizeof(t_rapidmax),
				  0L, A_GIMME, 0);
    
	class_addmethod(c, (method)rapidmax_assist,			"assist",		A_CANT, 0);

	class_register(CLASS_BOX, c);
    rapidmax_class = c;
    
    class_addmethod(c, (method)rapidmax_regress, "regress", A_GIMME, 0);
    class_addmethod(c, (method)rapidmax_classify, "classify", A_GIMME, 0);

    class_addmethod(c, (method)rapidmax_process_int, "int", A_LONG, 0);
    class_addmethod(c, (method)rapidmax_process_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)rapidmax_process_list, "list", A_GIMME, 0);
    
    class_addmethod(c, (method)rapidmax_initialize, "initialize", A_DEFSYM, 0);
    
    class_addmethod(c, (method)rapidmax_read, "read", A_DEFSYM, 0);
    class_addmethod(c, (method)rapidmax_write, "write", A_DEFSYM, 0);
    
    post("I am the rapidmax object");
}

void rapidmax_assist(t_rapidmax *x, void *b, long m, long a, char *s)
{

}

void rapidmax_free(t_rapidmax *x)
{
	;
}


void *rapidmax_new(t_symbol *s, long argc, t_atom *argv)
{
    
	t_rapidmax *x;
    
    x = (t_rapidmax *)object_alloc(rapidmax_class);
    x->m_outlet2 = bangout((t_object *)x);
    x->m_outlet1 = listout((t_object *)x);
    
    if(argc == 1)
    {
        object_post((t_object*)x, "Loading %s", atom_getsym(argv)->s_name);
        
        rapidmax_doread(x, atom_getsym(argv));
    }
    
    return (x);
}

void rapidmax_initialize(t_rapidmax *x)
{
    
    if(x->model != nullptr)
    {
        x->model->initialize();
        delete x->model;
        x->model = nullptr;
    }
    x->trainingSet.clear();
    x->trained = false;
    object_post((t_object*)x, "Object has been initialized, model deleted.");
}

void rapidmax_write(t_rapidmax *x, t_symbol *s)
{
    if(x->trained)
        defer(x, (method)rapidmax_dowrite, s, 0, NULL);
    else
        object_post((t_object*)x, "Model has not been trained");
}

void rapidmax_dowrite(t_rapidmax *x, t_symbol *s)
{
    short numtypes = 1;
    short path;
    
    std::string filename ("my");
    
    filename = filename + x->modeltype + "model.json";
    
    char *cfilename = new char[filename.length() + 1];
    strcpy(cfilename, filename.c_str());
    
    t_fourcc type;
    t_fourcc typelist = 'json';
    
    if (s == gensym("")) {      // if no argument supplied, ask for file
        if (saveasdialog_extended(cfilename, &path, &type, &typelist, numtypes))     // non-zero: user cancelled
        {
            return;
        }
    } else {
        strcpy(cfilename, s->s_name);
        path = path_getdefault();
    }
    
    char absoluteFilename[512];
    short fileAlreadyExists = 0;
    if(path_topotentialname(path, cfilename, absoluteFilename, fileAlreadyExists))
    {
        object_post((t_object*)x, "Can not write to %s", absoluteFilename);
        return;
    }
    else
    {
        if(fileAlreadyExists)
        {
            object_post((t_object*)x, "%s will be overwritten", absoluteFilename);
        }
    }

    std::string str(absoluteFilename);
    
    //Very crude file path formatting, couldn't find a good way of finding the name of the root folder to minus off the absolute path.
    str.erase(str.begin(), str.begin()+str.find_first_of(":")+1);
    
    object_post((t_object*)x, "Writing model to %s", str.data());

    x->model->writeJSON(str);
}

void rapidmax_read(t_rapidmax *x, t_symbol *s)
{
    defer(x, (method)rapidmax_doread, s, 0, NULL);
}

void rapidmax_doread(t_rapidmax *x, t_symbol *s)
{
    if(x->model == nullptr)
    {
        x->model = new regression();
    } else {
        rapidmax_initialize(x);
        x->model = new regression();
    }
    
    t_fourcc filetype = 'JSON', outtype;
    short numtypes = 1;
    char filename[MAX_PATH_CHARS];
    short path;
    if (s == gensym("")) {      // if no argument supplied, ask for file
        if (open_dialog(filename, &path, &outtype, &filetype, numtypes))       // non-zero: user cancelled
            return;
    } else {
        strcpy(filename, s->s_name);    // must copy symbol before calling locatefile_extended
        if (locatefile_extended(filename, &path, &outtype, &filetype, 1)) { // non-zero: not found
            object_post((t_object*)x, "%s: not found", s->s_name);
            return;
        }
    }
    
    char absoluteFilename[512];
    if(path_topathname(path, filename, absoluteFilename))
    {
        object_post((t_object*)x, "Could not find %s", absoluteFilename);
        return;
    }
    
    std::string str(absoluteFilename);
    
    //Very crude file path formatting, couldn't find a good way of finding the name of the root folder to minus off the absolute path.
    str.erase(str.begin(), str.begin()+str.find_first_of(":")+1);
    
    object_post((t_object*)x, "Opening %s", str.data());
    
    x->trained = x->model->readJSON(str);
    
    //object_post((t_object*)x, "Model is trained %d.", x->trained);
    
}

void rapidmax_process_int(t_rapidmax *x, long l)
{
    float f = (float)l;
    rapidmax_process(x, 1, &f);
}

void rapidmax_process_float(t_rapidmax *x, double f)
{
    float newf = (float)f;
    rapidmax_process(x, 1, &newf);
}

void rapidmax_process_list(t_rapidmax *x, t_symbol *s, long argc, t_atom *argv)
{
    
    float *vals = new float[argc];
    atom_getfloat_array (argc, argv, argc, vals);
    
    rapidmax_process(x, argc, vals);
    
    vals = nullptr;
    delete[] vals;
}

//User sends new data straight into object
void rapidmax_process(t_rapidmax *x, long argc, float *argv)
{
    
    //Check that models have been trained.
    if(!x->trained)
    {
//        object_post((t_object*)x, "Must train object before processing.");
        return;
    }
    
    //check input type are floats and convert to C++ vector
    long i;
    std::vector<double> inputData_double;
    float *ap;
    for(i =0, ap = argv; i<argc; i++, ap++)
    {
        inputData_double.push_back(*ap);
    }
    
    //process vector using rapid api.
    
    std::vector<double> prediction_double = x->model->process(inputData_double);
    
    //convert vector of doubles back into atom array of floats.
    
    std::vector<t_atom> prediction_atoms;
    
    for(auto d : prediction_double)
    {
        t_atom temp_atom_float;
        atom_setfloat(&temp_atom_float, d);
        prediction_atoms.push_back(temp_atom_float);
    }
    
    outlet_list(x->m_outlet1, gensym("float"), prediction_atoms.size(), prediction_atoms.data());
    
    //send atom array out of left outlet.
}

void rapidmax_regress(t_rapidmax *x, t_symbol *s, long argc, t_atom *argv)
{
    
    if(x->trained)
    {
        rapidmax_initialize(x);
    }
    
    //Check how many inputs were in the function call
    if(argc < 1 )
    {
        object_post((t_object*)x, "must take an argument");
        return;
    }
    
    x->model = new regression();
    x->modeltype = atom_getsym(argv)->s_name;
    
    x->modeltype = "regr";
    
    object_post((t_object*)x, "Model Type: %s", x->modeltype.data());
    
    rapidmax_train(x, s, argc, argv);
}

void rapidmax_classify(t_rapidmax *x, t_symbol *s, long argc, t_atom *argv)
{
    
    if(x->trained)
    {
        rapidmax_initialize(x);
    }
    
    //Check how many inputs were in the function call
    if(argc < 1 )
    {
        object_post((t_object*)x, "must take an argument");
        return;
    }
    
    x->model = new classification();
    x->modeltype = atom_getsym(argv)->s_name;
    
    x->modeltype = "clas";
    
    object_post((t_object*)x, "Model Type: %s", x->modeltype.data());
    
    rapidmax_train(x, s, argc, argv);
}

void rapidmax_train(t_rapidmax *x, t_symbol *s, long argc, t_atom *argv)
{
    long i, j;
    t_dictionary *incommingdict = NULL;
    
    //Check name of their dict object.
    if(atom_gettype(argv) != A_SYM)
    {
        object_post((t_object*)x, "Argument Type is Invalid (Must be SYMB)");
        return;
    }
    
    incommingdict = dictobj_findregistered_retain (atom_getsym(argv));
    object_post((t_object*)x, "Found dictionary %s", atom_getsym(argv)->s_name);
    
    //Tell em we got the dict.
    if(incommingdict == NULL)
    {
        object_post((t_object*)x, "Dictionary was not read");
        return;
    }
    
    t_symbol   **keys = NULL;
    long       numkeys = 0;
    dictionary_getkeys(incommingdict, &numkeys, &keys);
    
    object_post((t_object*)x, "Number of training examples in dict is %ld", numkeys);
    
    if(!numkeys)
    {
        object_post((t_object*)x, "Dictionary is empty");
        free_dict(incommingdict, numkeys, keys);
        return;
    }

    
    //Variables for subdictionaries.
    t_object *subdict_obj = nullptr;
    t_dictionary *subdict = nullptr;
    long       numsubkeys = 0;
    t_symbol   **subkeys = NULL;
    
    for(i = 0; i<numkeys; ++i)
    {
        
        //Make sure the elements of the dict are the right type.
        if(!dictionary_entryisdictionary(incommingdict, keys[i]))
        {
            object_post((t_object*)x, "Dictionary element %s is not a sub-dictionary", keys[i]->s_name);
            free_dict(incommingdict, numkeys, keys);
            return;
        }
        
        //Getting the subdict weird mapping at the point.
        dictionary_getdictionary (incommingdict, keys[i], &subdict_obj);
        subdict = (t_dictionary *)subdict_obj;
        
        //Getting the keys of the subdict.
        dictionary_getkeys(subdict, &numsubkeys, &subkeys);
        
        long    input_size = 0, output_size = 0;
        t_atom *input_atoms, *output_atoms = NULL;
        
        bool has_input = false, has_output = false;
        
        //Getting the atoms in the subdict
        for(j = 0; j<numsubkeys; j++)
        {
            if(strcmp(subkeys[j]->s_name,  "input") == 0)
            {
                dictionary_getatoms(subdict, subkeys[j], &input_size, &input_atoms);
                has_input = true;
            }
            if(strcmp(subkeys[j]->s_name,  "output") == 0)
            {
                dictionary_getatoms(subdict, subkeys[j], &output_size, &output_atoms);
                has_output = true;
            }
        }
        
        if(!has_input || !has_output)
        {
            object_post((t_object*)x, "Contence of sub-dicitionary %s does not have an input and output", keys[i]->s_name);
            free_dict(incommingdict, numkeys, keys);
            return;
        }
        
        if(i)
        {
            if(x->trainingSet[i-1].input.size() != input_size || x->trainingSet[i-1].output.size() != output_size)
            {
                object_post((t_object*)x, "Dimentions of sub-dicitionary %s's input or output are not consistent with the rest of the training data", keys[i]->s_name);
                free_dict(incommingdict, numkeys, keys);
                return;
            }
        }
        
        trainingExample te;
        if(rapidmax_fill_training_example(x, te.input, input_size, input_atoms) || rapidmax_fill_training_example(x, te.output,output_size, output_atoms))
        {
            object_post((t_object*)x, "Contence of sub-dicitionary %s::input or output was not of a readable type (Long, Float)", keys[i]->s_name);
            free_dict(incommingdict, numkeys, keys);
            return;
        }
        
        //Push the new training example into the global training set.
        x->trainingSet.push_back(te);
    }
    
    object_post((t_object*)x, "All training data is correctly formatted");
    free_dict(incommingdict, numkeys, keys);
    
    //TRAIN
    x->trained = x->model->train(x->trainingSet);
    
    if(x->trained)
    {
        object_post((t_object*)x, "Model is trained");
        outlet_bang(x->m_outlet2);
    }
    else
    {
        object_post((t_object*)x, "Model is not trained");
    }
}

int rapidmax_fill_training_example(t_rapidmax *x, std::vector<double> &v, long argc, t_atom *argv)
{
    t_atom *p;
    long i;
    for (i = 0, p = argv; i < argc; i++, p++)
    {
        switch (atom_gettype(p))
        {
            //Convert the atom into a type.
            case A_LONG:
            {
                v.push_back(atom_getlong(p));
                break;
            }
            case A_FLOAT:
            {
                v.push_back(atom_getfloat(p));
                break;
            }
            default:
            {
                return 1;
                break;
            }
        }
    }
    return 0;
}

void free_dict(t_dictionary *d, long numkeys, t_symbol **keys)
{
    if(numkeys)
        dictionary_freekeys(d, numkeys, keys);
    
    dictobj_release(d);
}



