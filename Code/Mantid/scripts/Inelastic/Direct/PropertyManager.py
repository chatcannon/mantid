from NonIDF_Properties import *

from collections import OrderedDict


class PropertyManager(NonIDF_Properties):
    """ Class defines the interface for Direct inelastic reduction with properties 
        present in Instrument_Properties.xml file

        These properties are responsible for fine turning up of the reduction

        Supported properties in IDF can be simple (prop[name]=value e.g. 
        prop['vanadium_mass']=30.5 
       
        or complex 
        where prop[name_complex_prop] value is equal [prop[name_1],prop[name_2]]
        e.g. time interval used in normalization on monitor 1:
        prop[norm_mon_integration_range] = [prop['norm-mon1-min'],prop['norm-mon1-max']]
        prop['norm-mon1-min']=1000,prop['norm-mon1-max']=2000

        There are properties which provide even more complex functionality. These properties have their own Descriptors. 

    
        The class is written to provide the following functionality. 

        1) Properties are initiated from Instrument_Properties.xml file as defaults. 
        2) Attempt to access property, not present in this file throws. 
        3) Attempt to create property not present in this file throws. 
        4) A standard behavior is defined for the most of the properties (get/set appropriate value) when there is number of 
           overloaded properties, which support more complex behavior using specially written attribute-classes. 
        5) Changes to the properties are recorded and list of changed properties is available on request


    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>

    """

    _class_wrapper ='_PropertyManager__';
    def __init__(self,Instrument,instr_run=None):
        #
        NonIDF_Properties.__init__(self,Instrument,instr_run);
        #
        # define private properties served the class
        private_properties = {'descriptors':[],'subst_dict':{},'prop_allowed_values':{},'changed_properties':set(),
        'file_properties':[],'abs_norm_file_properties':[]};
        # place these properties to __dict__  with proper decoration
        self._set_private_properties(private_properties);
        # ---------------------------------------------------------------------------------------------
        # overloaded descriptors. These properties have their personal descriptors, different from default
        all_methods = dir(self);
        self.__descriptors = prop_helpers.extract_non_system_names(all_methods,PropertyManager._class_wrapper);

        # retrieve the dictionary of property-values described in IDF
        param_list = prop_helpers.get_default_idf_param_list(self.instrument);

        param_list = self._convert_params_to_properties(param_list);
        #
        self.__dict__.update(param_list)


        # file properties -- the properties described files which should exist for reduction to work.
        self.__file_properties = ['det_cal_file','map_file','hard_mask_file']
        self.__abs_norm_file_properties = ['monovan_mapfile']

        # properties with allowed values
        self.__prop_allowed_values['normalise_method']=[None,'monitor-1','monitor-2','current']  # 'uamph', peak -- disabled/unknown at the moment
        self.__prop_allowed_values['deltaE_mode']=['direct'] # I do not think we should try other modes here


        # properties with special(overloaded and non-standard) setters/getters: Properties name dictionary returning tuple(getter,setter)
        # if some is None, standard one is used. 
        #self.__special_properties['monovan_integr_range']=(self._get_monovan_integr_range,lambda val : self._set_monovan_integr_range(val));
        # list of the parameters which should always be taken from IDF unless explicitly set from elsewhere. These parameters MUST have setters and getters
  
    def _convert_params_to_properties(self,param_list,detine_subst_dict=True):
        """ method processes parameters obtained from IDF and modifies the IDF properties
            to the form allowing them be assigned as python class properties.            
        """ 
            # build and use substitution dictionary
        if 'synonims' in param_list:
            synonyms_string  = param_list['synonims'];
            if detine_subst_dict:
                self.__subst_dict = prop_helpers.build_subst_dictionary(synonyms_string);
            #end
            # this dictionary will not be needed any more
            del param_list['synonims']
        #end


        # build and initiate  properties with default descriptors 
        #(TODO: consider complex automatic descriptors -- almost done differently through complex properties in dictionary)
        param_list =  prop_helpers.build_properties_dict(param_list,self.__subst_dict)

        #--------------------------------------------------------------------------------------
        # modify some IDF properties, which need overloaded getter (and this getter is provided somewhere among PropertiesDescriptors)
        if 'background_test_range' in param_list:
            val = param_list['background_test_range']
            param_list['_background_test_range'] = val;
            del param_list['background_test_range']
        else:
            param_list['_background_test_range'] = None;
        #end
        # make spectra_to_monitors_list to be the list indeed. 
        if 'spectra_to_monitors_list' in param_list:
            sml = SpectraToMonitorsList();
            param_list['spectra_to_monitors_list']=sml.convert_to_list(param_list['spectra_to_monitors_list'])
        #end
        #
        if 'monovan_integr_range' in param_list:
            # get reference to the existing class method
            param_list['_monovan_integr_range']=self.__class__.__dict__['monovan_integr_range']
            #
            val = param_list['monovan_integr_range']
            if str(val).lower() != 'none':
                prop= param_list['_monovan_integr_range']
                prop.__init__('AbsRange')
            del param_list['monovan_integr_range']
        #End monovan_integr_range
        #-
        # End modify. 
        #----------------------------------------------------------------------------------------
        return param_list

    def _set_private_properties(self,prop_dict):

        result = {};
        for key,val  in prop_dict.iteritems():
            new_key = self._class_wrapper+key;
            result[new_key]  = val;

        self.__dict__.update(result);

    @staticmethod
    def _is_private_property(prop_name):
        """ specify if property is private """
        if prop_name[:len(PropertyManager._class_wrapper)] == PropertyManager._class_wrapper :
            return True
        else:
            return False
        #end if


    def __setattr__(self,name0,val):
        """ Overloaded generic set method, disallowing non-existing properties being set.
               
           It also provides common checks for generic properties groups """ 

        if self._is_private_property(name0):
            self.__dict__[name0] = val;
            return

        if name0 in self.__subst_dict:
            name = self.__subst_dict[name0]
        else:
            name =name0;
        #end

        # replace common substitutions for string value
        if type(val) is str :
           val1 = val.lower()
           if (val1 == 'none' or len(val1) == 0):
              val = None;
           if val1 == 'default':
              val = self.getDefaultParameterValue(name0);
           # boolean property?
           if val1 in ['true','yes']:
               val = True
           if val1 in ['false','no']:
               val = False


        if type(val) is list and len(val) == 0:
            val = None;
 

        # Check allowed values property if value allowed
        if name in self.__prop_allowed_values:
            allowed_values = self.__prop_allowed_values[name];
            if not(val in allowed_values):
                raise KeyError(" Property {0} can not have value: {1}".format(name,val));
        #end

        # set property value:
        if name in self.__descriptors:
            try:
                super(PropertyManager,self).__setattr__(name,val)
            except AttributeError:
                raise AttributeError(" Can not set property {0} to value {1}".format(name,val));
        else:
            other_prop=prop_helpers.gen_setter(self.__dict__,name,val);
            #if other_prop:
            #    for prop_name in other_prop:
            #        self.__changed_properties.add(prop_name);

        # record changes in the property
        self.__changed_properties.add(name);

   # ----------------------------
    def __getattr__(self,name):
       """ Overloaded get method, disallowing non-existing properties being get but allowing 
          a property been called with  different names specified in substitution dictionary. """ 

       tDict = object.__getattribute__(self,'__dict__');
       if name is '__dict__':
           return tDict;
       else:
           if name in self.__subst_dict:
                name = self.__subst_dict[name]
           #end
           if name in self.__descriptors:
              ph = tDict['_'+name]           
              return ph.__get__(self)
           else:
              return prop_helpers.gen_getter(tDict,name)
       pass
#----------------------------------------------------------------------------------
#              Overloaded setters/getters
#----------------------------------------------------------------------------------
    #
    van_rmm = VanadiumRMM()
    #
    det_cal_file    = DetCalFile()
    #
    map_file        = MapMaskFile('map_file','.map',"Spectra to detector mapping file for the sample run")
    #
    monovan_mapfile = MapMaskFile('monovan_mapfile','.map',"Spectra to detector mapping file for the monovanadium integrals calculation")
    #
    hard_mask_file  = MapMaskFile('hard_mask_file','.msk',"Hard mask file")
    #
    monovan_integr_range     = MonovanIntegrationRange()
    #
    spectra_to_monitors_list = SpectraToMonitorsList()
    # 
    save_format = SaveFormat()
    #
    hardmaskOnly = HardMaskOnly()
    hardmaskPlus = HardMaskPlus()
    #
    diag_spectra = DiagSpectra()
    #
    background_test_range = BackbgroundTestRange()

#----------------------------------------------------------------------------------------------------------------
    def getChangedProperties(self):
        """ method returns set of the properties changed from defaults """
        return self.__dict__[self._class_wrapper+'changed_properties'];
    def setChangedProperties(self,value=set()):
        """ Method to clear changed properties list""" 
        if isinstance(value,set):
            self.__dict__[self._class_wrapper+'changed_properties'] =value;
        else:
            raise KeyError("Changed properties can be initialized by appropriate properties set only")

    @property
    def relocate_dets(self) :
        if self.det_cal_file != None:
            return True
        else:
            return False
  
    def set_input_parameters_ignore_nan(self,**kwargs):
        """ Like similar method set_input_parameters this one is used to 
            set changed parameters from dictionary of parameters. 

            Unlike set_input_parameters, this method does not set parameter, 
            with value  equal to None. As such, this method is used as interface to 
            set data from a function with a list of given parameters (*args vrt **kwargs),
            with some parameters missing.
        """   
        for par_name,value in kwargs.items() :
            if not(value is None):
                setattr(self,par_name,value);



    def set_input_parameters(self,**kwargs):
        """ Set input properties from a dictionary of parameters

        """

        for par_name,value in kwargs.items() :
            setattr(self,par_name,value);

        return self.getChangedProperties();

    def get_diagnostics_parameters(self):
        """ Return the dictionary of the properties used in diagnostics with their values defined in IDF
            
            if some values are not in IDF, default values are used instead
        """ 

        diag_param_list ={'tiny':1e-10, 'huge':1e10, 'samp_zero':False, 'samp_lo':0.0, 'samp_hi':2,'samp_sig':3,\
                           'van_out_lo':0.01, 'van_out_hi':100., 'van_lo':0.1, 'van_hi':1.5, 'van_sig':0.0, 'variation':1.1,\
                           'bleed_test':False,'bleed_pixels':0,'bleed_maxrate':0,\
                           'hard_mask_file':None,'use_hard_mask_only':False,'background_test_range':None,\
                           'instr_name':'','print_diag_results':True}
        result = {};

        for key,val in diag_param_list.iteritems():
            try:
                result[key] = getattr(self,key);
            except KeyError: 
                self.log('--- Diagnostics property {0} is not found in instrument properties. Default value: {1} is used instead \n'.format(key,value),'warning')
  
        return result;

    def update_defaults_from_instrument(self,pInstrument,ignore_changes=False):
        """ Method used to update default parameters from the same instrument (with different parameters).

            Used if initial parameters correspond to instrument with one validity dates and 
            current instrument has different validity dates and different default values for 
            these dates.

            List of synonims is not modified and new properties are not added assuming that 
            recent dictionary and properties are most comprehensive one

            ignore_changes==True when changes, caused by setting properties from instrument are not recorded
            ignore_changes==False -- getChangedProperties properties after applied this method would return set 
                            of all properties changed when applying this method

        """ 
        if self.instr_name != pInstrument.getName():
            self.log("*** WARNING: Setting reduction properties of the instrument {0} from the instrument {1}.\n"
                     "*** This only works if both instruments have the same reduction properties!"\
                      .format(self.instr_name,pInstrument.getName()),'warning')

        old_changes  = self.getChangedProperties()
        self.setChangedProperties(set())

        # find all changes, present in the old changes list
        existing_changes = old_changes.copy()
        for change in old_changes:
            dependencies = None
            try:
                prop = self.__class__.__dict__[change]
                dependencies = prop.dependencies()
            except:
                pass
            if dependencies:
                 existing_changes.update(dependencies)

        param_list = prop_helpers.get_default_idf_param_list(pInstrument)
        param_list =  self._convert_params_to_properties(param_list,False)

        #sort parameters to have complex properties (with underscore _) first    
        sorted_param =  OrderedDict(sorted(param_list.items(),key=lambda x : ord((x[0][0]).lower())))
  

        for key,val in sorted_param.iteritems():
            # set new values to old values and record this
            if key[0] == '_':
               public_name = key[1:]
            else:
               public_name = key
            # complex properties were modified to start with _
            if not(key in self.__dict__):
                 name = '_'+key
            else:
                 name = key

            try: # this is reliability check, and except ideally should never be hit. May occur if old IDF contains 
                    # properties, not present in recent IDF.
                  cur_val = self.__dict__[name]
            except:
                  self.log("property {0} or its derivatives have not been found in existing IDF. Ignoring this property"\
                       .format(key),'warning')
                  continue

            # complex properties may be set up through their members so no need to set up one
            if isinstance(cur_val,prop_helpers.ComplexProperty):
               # is complex property changed through its dependent properties?
               dependent_prop = val.dependencies()
               replace_old_value = True
               if public_name in existing_changes:
                   replace_old_value = False

               if replace_old_value: # may be property have changed through its dependencies
                    for prop_name in dependent_prop:
                        if  prop_name in existing_changes:
                            replace_old_value =False
                            break
               #
               if replace_old_value:
                   old_val = cur_val.__get__(self.__dict__)
                   new_val = val.__get__(param_list)
                   if old_val != new_val:
                      setattr(self,public_name,new_val)
               # remove dependent values from list of changed properties not to assign them later one-by one
               for prop_name in dependent_prop:
                   try:
                      del sorted_param[prop_name]
                   except:
                       pass
            # simple property
            else: 
                if public_name in existing_changes:
                    continue
                else: 
                   old_val = getattr(self,name);
                   if not(val == old_val or val == cur_val):
                     setattr(self,name,val)
        #End_if

        # Clear changed properties list (is this wise?, may be we want to know that some defaults changed?)
        if ignore_changes:
            self.setChangedProperties(old_changes)
            all_changes = old_changes
        else:
            new_changes = self.getChangedProperties()
            all_changes = old_changes.union(new_changes)
            self.setChangedProperties(all_changes)

        n=funcreturns.lhs_info('nreturns')
        if n>0:
            return all_changes
        else:
            return None;
    #end

    # TODO: finish refactoring this. 
    def init_idf_params(self, reinitialize_parameters=False):
        """
        Initialize some default parameters and add the one from the IDF file
        """
        if self._idf_values_read == True and reinitialize_parameters == False:
            return

        """
        Attach analysis arguments that are particular to the ElasticConversion

        specify some parameters which may be not in IDF Parameters file
        """

          # should come from Mantid
        # Motor names-- SNS stuff -- psi used by nxspe file
        # These should be reconsidered on moving into _Parameters.xml
        self.motor = None
        self.motor_offset = None

        if self.__facility == 'SNS':
            self.normalise_method  = 'current'



    def _check_file_exist(self,file_name):
        file_path = FileFinder.getFullPath(file);
        if len(file_path) == 0:
            try:
                file_path = common.find_file(file)
            except:
                file_path ='';


    def _check_necessary_files(self,monovan_run):
        """ Method verifies if all files necessary for a run are available.

           useful for long runs to check if all files necessary for it are present/accessible
        """

        def check_files_list(files_list):
            file_missing = False
            for prop in files_list :
                file = getattr(self,prop)
                if not (file is None) and isinstance(file,str):
                    file_path = self._check_file_exist(file);
                    if len(file_path)==0:
                       self.log(" Can not find file ""{0}"" for property: {1} ".format(file,prop),'error')
                       file_missing=True

            return file_missing

        base_file_missing = check_files_list(self.__file_properties)
        abs_file_missing=False
        if not (monovan_run is None):
            abs_file_missing = check_files_list(self.__abs_norm_file_properties)

        if  base_file_missing or abs_file_missing:
             raise RuntimeError(" Files needed for the run are missing ")
    #
    def _check_monovan_par_changed(self):
        """ method verifies, if properties necessary for monovanadium reduction have indeed been changed  from defaults """

        # list of the parameters which should usually be changed by user and if not, user should be warn about it.
        momovan_properties=['sample_mass','sample_rmm','monovan_run']
        changed_prop = self.getChangedProperties();
        non_changed = [];
        for property in momovan_properties:
            if not property in changed_prop:
                non_changed.append(property)
        return non_changed;

    #
    def log_changed_values(self,log_level='notice',display_header=True,already_changed=set()):
      """ inform user about changed parameters and about the parameters that should be changed but have not
      
        This method is abstract method of NonIDF_Properties but is fully defined in PropertyManager

        display_header==True prints nice additional information about run. If False, only 
        list of changed properties displayed.
      """
      if display_header:
        # we may want to run absolute units normalization and this function has been called with monovan run or helper procedure
        if self.monovan_run != None :
            # check if mono-vanadium is provided as multiple files list or just put in brackets occasionally
            self.log("****************************************************************",'notice');
            self.log('*** Output will be in absolute units of mb/str/mev/fu','notice')
            non_changed = self._check_monovan_par_changed();
            if len(non_changed) > 0:
                for prop in non_changed:
                    value = getattr(self,prop)
                    message = "\n***WARNING!: Absolute units reduction parameter : {0} has its default value: {1}"\
                              "\n             This may need to change for correct absolute units reduction\n"

                    self.log(message.format(prop,value),'warning')


          # now let's report on normal run.
        self.log("*** Provisional Incident energy: {0:>12.3f} mEv".format(self.incident_energy),log_level)
      #end display_header

      self.log("****************************************************************",log_level);
      changed_Keys= self.getChangedProperties();
      for key in changed_Keys:
          if key in already_changed:
              continue
          val = getattr(self,key);
          self.log("  Value of : {0:<25} is set to : {1:<20} ".format(key,val),log_level)

      if not display_header:
          return

      save_dir = config.getString('defaultsave.directory')
      self.log("****************************************************************",log_level);
      if self.monovan_run != None and not('van_mass' in changed_Keys):                           # This output is 
         self.log("*** Monochromatic vanadium mass used : {0} ".format(self.van_mass),log_level) # Adroja request from may 2014
      #
      self.log("*** By default results are saved into: {0}".format(save_dir),log_level);
      self.log("*** Output will be normalized to {0}".format(self.normalise_method),log_level);
      if  self.map_file == None:
            self.log('*** one2one map selected',log_level)
      self.log("****************************************************************",log_level);


    #def help(self,keyword=None) :
    #    """function returns help on reduction parameters.

    #       if provided without arguments it returns the list of the parameters available
    #    """
    #    raise KeyError(' Help for this class is not yet implemented: see {0}_Parameter.xml in the file for the parameters description'.format());

if __name__=="__main__":
    pass


