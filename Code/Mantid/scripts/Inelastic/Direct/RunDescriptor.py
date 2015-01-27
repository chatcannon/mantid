""" File contains Descriptors used describe run for direct inelastic reduction """ 


from mantid.simpleapi import *
from PropertiesDescriptors import *


class RunDescriptor(PropDescriptor):
    """ descriptor supporting a run and a workspace  """

    # the host class referencing contained all instantiated descriptors. 
    # Descriptors methods rely on it to work (e.g. to extract file loader preferences) 
    # so it has to be set up manually by PropertyManager __init__ method
    _holder=None
    _logger = None
#--------------------------------------------------------------------------------------------------------------------
    def __init__(self,prop_name,DocString=None): 
        """ """
        # Run number 
        self._run_number  = None
        # Extension of the file to load data from
        # 
        self._prop_name = prop_name
        self._run_file_path = ''
        self._run_ext     = None
        # Workspace name which corresponds to the run 
        self._ws_name = None
        # String used to identify the workspace related to this property w.r.t. other workspaces
        self._ws_cname  = ''
        self._ws_suffix   = ''
        self._runs_to_add = None
        #
        if not DocString is None:
            self.__doc__ = DocString
#--------------------------------------------------------------------------------------------------------------------
    def __get__(self,instance=None,owner=None):
       """ return current run number or workspace if it is loaded""" 
       if instance is None:
           return self
       if self._ws_name and self._ws_name in mtd:
           return mtd[self._ws_name]
       else:
           return self._run_number 
#--------------------------------------------------------------------------------------------------------------------
    def __set__(self,instance,value):
       """ Set up Run number and define workspace name from any source """
       # 
       old_ws_name = self._ws_name
       clear_add_list=True
       #end
       if value == None: # clear current run number
           self._run_number  = None
           self._ws_name     = None
           self._ws_cname    = ''
           self._ws_suffix   = ''   
           self._clear_old_ws(old_ws_name,self._ws_name)
           return
       if isinstance(value, api.Workspace):
           if 'SumOfRuns:'  in value.getRun():
               pass # TODO: not implemented
           else:
                self._run_number = value.getRunNumber()
                ws_name = value.name()
                self._split_ws_name(ws_name)
                self.synchronize_ws(value)
                self._clear_old_ws(old_ws_name,self._ws_name)
           return

       if isinstance(value,str): # it may be run number as string or it may be a workspace name
          if value in mtd: # workspace name
              ws = mtd[value]
              self.__set__(instance,ws)
              return
          else:
              file_path,run_num,fext = prop_helpers.parse_run_file_name(value)

              if isinstance(run_num,list):
                  self.__set__(instance,run_num)
                  n_to_add = len(self._runs_to_add[1])
                  self._runs_to_add=(file_path[:n_to_add],run_num[:n_to_add],fext[:n_to_add])
                  main_fext = fext[0].lower()
                  clear_add_list=False
              else:
                  self._run_number    = run_num
                  self._run_file_path = file_path
                  main_fext     = fext.lower()
                  clear_add_list=True

              if len(main_fext)>0:
                    self._run_ext = main_fext
              else:
                    self._run_ext = None


       elif isinstance(value,list):
           if isinstance(instance.sum_runs,bool) and instance.sum_runs:
                self._run_number = value[0]
                self._runs_to_add=([""]*len(value),value,[""]*len(value))
                clear_add_list = False
           elif(isinstance(instance.sum_runs,int) and instance.sum_runs>0):
               num_2sum = instance.sum_runs
               if num_2sum > len(value):
                   raise ValueError("Requested to sum {0} runs but provided the list of only {1} runs".format(num_2sum,len(value)))
               sublist  = value[0:num_2sum]

               self._run_number = value[0]
               self._runs_to_add=([""]*num_2sum,sublist,[""]*num_2sum)
               clear_add_list = False
           else:
               self._run_number = value[0]
               clear_add_list = True

       else:
           self._run_number = int(value)

       self._ws_cname  = ''
       self._ws_name = None
       self._clear_old_ws(old_ws_name,None,clear_add_list)
#--------------------------------------------------------------------------------------------------------------------    
    def run_number(self):
        """ Return run number regardless of workspace is loaded or not"""
        if self._ws_name and self._ws_name in mtd:
            ws = mtd[self._ws_name]
            return ws.getRunNumber()
        else:
            return self._run_number 
#--------------------------------------------------------------------------------------------------------------------    
    def is_monws_separate(self):
        """ """
        mon_ws = self.get_monitors_ws()
        name   = mon_ws.name()
        if name.endswith('_monitors'):
            return True
        else:
            return False

#--------------------------------------------------------------------------------------------------------------------    
    def set_action_suffix(self,suffix=None):
        """ method to set part of the workspace name, which indicate some action performed over this workspace           
            
            e.g.: default suffix of a loaded workspace is 'RAW' but we can set it to SPE to show that conversion to 
            energy will be performed for this workspace.

            method returns the name of the workspace is will have with this suffix. Algorithms would later  
            work on the initial workspace and modify it in-place or to produce workspace with new name (depending if one 
            wants to keep initial workspace)
            synchronize_ws(ws_pointer) then should synchronize workspace and its name.

            TODO: This method should be automatically invoked by an algorithm decorator
            Until implemented, one have to ensure that it is correctly used together with synchronize_ws
            to ensue one can always get workspace from its name
        """
        if suffix:
            self._ws_suffix = suffix
        else: # return to default
            self._ws_suffix=''
        return self._build_ws_name()
#--------------------------------------------------------------------------------------------------------------------
    def synchronize_ws(self,workspace=None):
        """ Synchronize workspace name (after workspace may have changed due to algorithm) 
            with internal run holder name. Accounts for the situation when 

            TODO: This method should be automatically invoked by an algorithm decorator
            Until implemented, one have to ensure that it is correctly used together with 
            set_action_suffix to ensue one can always get expected workspace from its name
            outside of a method visibility 
        """ 
        if not workspace:
            workspace=mtd[self._ws_name]

        new_name = self._build_ws_name()
        old_name = workspace.name()
        if new_name != old_name:
            if new_name in mtd:
               DeleteWorkspace(new_name)
            RenameWorkspace(InputWorkspace=old_name,OutputWorkspace=new_name)

            old_mon_name = old_name+'_monitors'
            new_mon_name = new_name+'_monitors'
            if new_mon_name in mtd:
               DeleteWorkspace(new_mon_name)
            if old_mon_name in mtd:
               RenameWorkspace(InputWorkspace=old_mon_name,OutputWorkspace=new_mon_name)
        self._ws_name = new_name
#--------------------------------------------------------------------------------------------------------------------    
    def get_file_ext(self):
        """ Method returns current file extension for file to load workspace from 
            e.g. .raw or .nxs extension
        """ 
        if self._run_ext:
            return self._run_ext
        else: # return IDF default
            return RunDescriptor._holder.data_file_ext
#--------------------------------------------------------------------------------------------------------------------
    def set_file_ext(self,val):
        """ set non-default file extension """
        if isinstance(val,str):
            if val[0] != '.':
                value = '.' + val
            else:
                value = val
            self._run_ext = value
        else:
            raise AttributeError('Source file extension can be only a string')

    def file_hint(self,run_num_str,filePath=None,fileExt=None,**kwargs):
        """ procedure to provide run file guess name from run properties
         
            main purpose -- to support customized order of file extensions
        """ 

        inst_name = RunDescriptor._holder.short_inst_name
        if 'file_hint' in kwargs:
            hint = kwargs['file_hint']
            fname,old_ext=os.path.splitext(hint)
            if len(old_ext) == 0:
                old_ext = self.get_file_ext()
        else:
            if fileExt:
               old_ext = fileExt
            else:
               old_ext = self.get_file_ext()

            hint =inst_name + run_num_str + old_ext
            if not filePath:
                filePath = self._run_file_path
            if os.path.exists(filePath):
                hint = os.path.join(filePath,hint)
        if os.path.exists(hint):
            return hint,old_ext
        else:
            fp,hint=os.path.split(hint)
        return hint,old_ext

#--------------------------------------------------------------------------------------------------------------------
    def find_file(self,run_num = None,filePath=None,fileExt=None,**kwargs):
        """Use Mantid to search for the given run. """

        inst_name = RunDescriptor._holder.short_inst_name
        if run_num:
            run_num_str = str(run_num)
        else:
            run_num_str = str(self.run_number())
        #
        file_hint,old_ext = self.file_hint(run_num_str,filePath,fileExt,**kwargs)

        try:
            file = FileFinder.findRuns(file_hint)[0]
            fname,fex=os.path.splitext(file)
            self._run_ext = fex
            if old_ext != fex:
                message='   Cannot find run-file with extension {0}.\n'\
                        '   Found file {1} instead'.format(old_ext,file)
                RunDescriptor._logger(message,'notice')
            self._run_file_path = os.path.dirname(fname)
            return file
        except RuntimeError:
             message = 'Cannot find file matching hint {0} on current search paths ' \
                       'for instrument {1}'.format(file_hint,inst_name)

             RunDescriptor._logger(message,'warning')
             return 'ERROR:find_file: '+message
#--------------------------------------------------------------------------------------------------------------------
    @staticmethod
    def _check_claibration_source():
         """ if user have not specified calibration as input to the script, 
             try to retrieve calibration stored in file with run properties"""
         changed_prop = RunDescriptor._holder.getChangedProperties()
         if 'det_cal_file' in changed_prop:
              use_workspace_calibration = False
         else:
              use_workspace_calibration = True
         return use_workspace_calibration

    def get_workspace(self):
        """ Method returns workspace correspondent to current run number(s)
            and loads this workspace if it has not been loaded

            Returns Mantid pointer to the workspace, corresponding to this run number
        """ 
        if not self._ws_name:
           self._ws_name = self._build_ws_name()


        if self._ws_name in mtd:
            ws = mtd[self._ws_name]
            if ws.run().hasProperty("calibrated"):
                return ws # already calibrated
            else:
               prefer_ws_calibration = self._check_claibration_source()
               self.apply_calibration(ws,RunDescriptor._holder.det_cal_file,prefer_ws_calibration)
               return ws
        else:
           if self._run_number:
               prefer_ws_calibration = self._check_claibration_source()
               inst_name   = RunDescriptor._holder.short_inst_name
               calibration = RunDescriptor._holder.det_cal_file
               if not self._runs_to_add:
                   ws = self.load_run(inst_name, calibration,False, RunDescriptor._holder.load_monitors_with_workspace,prefer_ws_calibration)
               else: # Sum runs
                   file_guess,run_list,fext = self._runs_to_add

                   RunDescriptor._logger("*** Summing multiple runs            ****",'notice')
                   RunDescriptor._logger("*** Loading run N: {0} ".format(self._run_number),'notice')
                   mon_with_ws = RunDescriptor._holder.load_monitors_with_workspace
                   ws = self.load_run(inst_name, None,False,mon_with_ws,False,file_guess[0],fext[0])
                   sum_ws_name = ws.name()
                   sum_mon_name = sum_ws_name+'_monitors'

                   AddedRunNumbers = str(self.run_number())
                   numTerms = len(run_list)
                   for ind,run_num in enumerate(run_list[1:]):
                       RunDescriptor._logger("*** Adding  run N: {0} ".format(run_num),'notice')

                       term_name = '{0}_ADDITIVE_#{1}/{2}'.format(inst_name,ind+2,numTerms)#
                       file_h    = os.path.join(file_guess[ind+1],'{0}{1}{2}'.format(inst_name,run_num,fext[ind+1]))

                       wsp       =  self.load_run(inst_name, None,False, mon_with_ws,False,file_hint=file_h,ws_name = term_name)

                       wsp_name = wsp.name()
                       Plus(LHSWorkspace=sum_ws_name,RHSWorkspace=wsp_name,OutputWorkspace=sum_ws_name,ClearRHSWorkspace=True)
                       AddedRunNumbers+=',{0}'.format(run_num)
                       if not mon_with_ws:
                          Plus(LHSWorkspace=sum_mon_name,RHSWorkspace=wsp_name+'_monitors',OutputWorkspace=sum_mon_name,ClearRHSWorkspace=True)
                   RunDescriptor._logger("*** Summing multiple runs  completed ****",'notice')

                   ws = mtd[sum_ws_name]
                   self.synchronize_ws(ws)

                   self.apply_calibration(ws,calibration,prefer_ws_calibration)

                   AddSampleLog(Workspace=sum_ws_name,LogName = 'SumOfRuns:',LogText=AddedRunNumbers,LogType='String')

               return ws
           else:
              return None
#--------------------------------------------------------------------------------------------------------------------
    def get_ws_clone(self,clone_name='ws_clone'):
        """ Get unbounded clone of existing Run workspace """
        ws = self.get_workspace()
        CloneWorkspace(InputWorkspace=ws,OutputWorkspace=clone_name)
        mon_ws_name  = self.get_ws_name()+'_monitors'
        if mon_ws_name in mtd:
            cl_mon_name = clone_name+'_monitors'
            CloneWorkspace(InputWorkspace=mon_ws_name,OutputWorkspace=cl_mon_name)

        return mtd[clone_name]
#--------------------------------------------------------------------------------------------------------------------
    def get_monitors_ws(self,monitor_ID = None):
        """ get pointer to a workspace containing monitors. 

           Explores different ways of finding monitor workspace in Mantid and returns the python pointer to the
           workspace which contains monitors.
        """
        data_ws = self.get_workspace()

        monWS_name = self.get_ws_name()+'_monitors'
        if monWS_name in mtd:
            mon_ws = mtd[monWS_name]
            monitors_separate = True
        else:
            mon_ws = data_ws
            monitors_separate = False

        spec_to_mon =   RunDescriptor._holder.spectra_to_monitors_list
        if monitors_separate and spec_to_mon :
            for specID in spec_to_mon:
                mon_ws=self.copy_spectrum2monitors(data_ws,mon_ws,specID)

        if monitor_ID:
           try:
               ws_index = mon_ws.getIndexFromSpectrumNumber(monitor_ID) 
           except: # 
               mon_ws = None
        #else: #TODO: get list of monitors and verify that they are indeed in monitor workspace

        return mon_ws
#--------------------------------------------------------------------------------------------------------------------
    def get_ws_name(self):
        """ return workspace name. If ws name is not defined, build it first and set up as the target ws name
        """ 

        if self._ws_name:
            if self._ws_name in mtd:
                return self._ws_name
            else:
                raise RuntimeError('Getting workspace name {0} for undefined workspace. Run get_workspace first'.format(self._ws_name)) 



        self._ws_name = self._build_ws_name()
        return self._ws_name
#--------------------------------------------------------------------------------------------------------------------
    def load_run(self,inst_name, calibration=None, force=False, load_mon_with_workspace=False,use_ws_calibration=True,\
                 filePath=None,fileExt=None,**kwargs):
        """Loads run into the given workspace.

           If force is true then the file is loaded regardless of whether  its workspace exists already
        """
        # If a workspace with this name exists, then assume it is to be used in place of a file
        if 'ws_name' in kwargs:
            ws_name = kwargs['ws_name']
        else:
            try:
                ws_name = self.get_ws_name()
            except RuntimeError:
                self._ws_name=None
                ws_name = self.get_ws_name()
        #-----------------------------------
        if ws_name in mtd and not(force):
            RunDescriptor._logger("{0} already loaded as workspace.".format(ws_name),'information')
        else:
            # If it doesn't exists as a workspace assume we have to try and load a file
            data_file = self.find_file(None,filePath,fileExt,**kwargs)
            if data_file[0:4] == 'ERROR':
                raise IOError(data_file)
                       
            if load_mon_with_workspace:
                   mon_load_option = 'Include'
            else:
                   mon_load_option = 'Separate'
            #
            try: # Hack LoadEventNexus does not understand Separate at the moment and throws
                Load(Filename=data_file, OutputWorkspace=ws_name,LoadMonitors = mon_load_option)
            except ValueError:
                #mon_load_option =str(int(load_mon_with_workspace))
                Load(Filename=data_file, OutputWorkspace=ws_name,LoadMonitors = '1')


            RunDescriptor._logger("Loaded {0}".format(data_file),'information')
        #end

        loaded_ws = mtd[ws_name]
        ######## Now we have the workspace
        self.apply_calibration(loaded_ws,calibration,use_ws_calibration)
        return loaded_ws
#--------------------------------------------------------------------------------------------------------------------
    def apply_calibration(self,loaded_ws,calibration=None,use_ws_calibration=True):
        """  If calibration is present, apply it to the workspace 
        
             use_ws_calibration -- if true, retrieve workspace property, which defines 
             calibration option (e.g. det_cal_file used a while ago) and try to use it
        """

        if not (calibration or use_ws_calibration):
            return 
        if not isinstance(loaded_ws, api.Workspace):
           raise RuntimeError(' Calibration can be applied to a workspace only and got object of type {0}'.format(type(loaded_ws)))
        
        if loaded_ws.run().hasProperty("calibrated"):
            return # already calibrated

        ws_calibration = calibration
        if use_ws_calibration:
            try:
                ws_calibration = prop_helpers.get_default_parameter(loaded_ws.getInstrument(),'det_cal_file')
                if ws_calibration is None:
                    ws_calibration = calibration
                if isinstance(ws_calibration,str) and ws_calibration.lower() == 'none':
                    ws_calibration = calibration
                if ws_calibration :
                    test_name = ws_calibration
                    ws_calibration = FileFinder.getFullPath(ws_calibration)
                    if len(ws_calibration) == 0:
                       raise RuntimeError('Can not find defined in run {0} calibration file {1}\n'\
                                          'Define det_cal_file reduction parameter properly'.format(loaded_ws.name(),test_name))
                    RunDescriptor._logger('*** load_data: Calibrating data using workspace defined calibration file: {0}'.format(ws_calibration),'notice')
            except KeyError: # no det_cal_file defined in workspace
                if calibration:
                    ws_calibration = calibration
                else:
                    return

        if type(ws_calibration) == str : # It can be only a file (got it from calibration property)
            RunDescriptor._logger('load_data: Moving detectors to positions specified in cal file {0}'.format(ws_calibration),'debug')
            # Pull in pressures, thicknesses & update from cal file
            LoadDetectorInfo(Workspace=loaded_ws, DataFilename=ws_calibration, RelocateDets=True)
            AddSampleLog(Workspace=loaded_ws,LogName="calibrated",LogText=str(ws_calibration))
        elif isinstance(ws_calibration, api.Workspace):
            RunDescriptor._logger('load_data: Copying detectors positions from workspace {0}: '.format(ws_calibration.name()),'debug')
            CopyInstrumentParameters(InputWorkspace=ws_calibration,OutputWorkspace=loaded_ws)
            AddSampleLog(Workspace=loaded_ws,LogName="calibrated",LogText=str(ws_calibration))
#--------------------------------------------------------------------------------------------------------------------
    @staticmethod
    def copy_spectrum2monitors(data_ws,mon_ws,spectraID):
       """
        this routine copies a spectrum form workspace to monitor workspace and rebins it according to monitor workspace binning

        @param data_ws  -- the  event workspace which detector is considered as monitor or Mantid pointer to this workspace
        @param mon_ws   -- the  histogram workspace with monitors where one needs to place the detector's spectra 
        @param spectraID-- the ID of the spectra to copy.

       """
 
       # ----------------------------
       try:
           ws_index = mon_ws.getIndexFromSpectrumNumber(spectraID)
           # Spectra is already in the monitor workspace
           return mon_ws
       except:
           ws_index = data_ws.getIndexFromSpectrumNumber(spectraID)

       #
       x_param = mon_ws.readX(0)
       bins = [x_param[0],x_param[1]-x_param[0],x_param[-1]]
       ExtractSingleSpectrum(InputWorkspace=data_ws,OutputWorkspace='tmp_mon',WorkspaceIndex=ws_index)
       Rebin(InputWorkspace='tmp_mon',OutputWorkspace='tmp_mon',Params=bins,PreserveEvents='0')
       # should be vice versa but Conjoin invalidate ws pointers and hopefully nothing could happen with workspace during conjoining
       #AddSampleLog(Workspace=monWS,LogName=done_log_name,LogText=str(ws_index),LogType='Number')
       mon_ws_name = mon_ws.getName()
       ConjoinWorkspaces(InputWorkspace1=mon_ws,InputWorkspace2='tmp_mon')
       mon_ws =mtd[mon_ws_name]

       if 'tmp_mon' in mtd:
           DeleteWorkspace(WorkspaceName='tmp_mon')
       return mon_ws
#--------------------------------------------------------------------------------------------------------------------
    def _sum_ext(self):
        if self._runs_to_add:
            sum_ext= "SumOf{0}".format(len(self._runs_to_add[1]))
        else:
            sum_ext = ''
        return sum_ext

    def _build_ws_name(self):

        instr_name  = self._instr_name()

        sum_ext = self._sum_ext()
        if self._run_number:
            ws_name  =  '{0}{1}{2}{3:0>#6d}{4}{5}'.format(self._prop_name,instr_name,self._ws_cname,self._run_number,sum_ext,self._ws_suffix)
        else:
            ws_name = '{0}{1}{2}{3}'.format(self._prop_name,self._ws_cname,sum_ext,self._ws_suffix)

        return ws_name 
    @staticmethod
    def rremove(thestr, trailing):
        thelen = len(trailing)
        if thestr[-thelen:] == trailing:
            return thestr[:-thelen]
        return thestr
    def _split_ws_name(self,ws_name):
        """ Method to split existing workspace name 
            into parts, in such a way that _build_name would restore the same name
        """
        # Remove suffix
        name = self.rremove(ws_name,self._ws_suffix)
        sumExt = self._sum_ext()
        if len(sumExt)>0:
            name = self.rremove(ws_name,sumExt)
        # remove _prop_name:
        name= name.replace(self._prop_name,'',1)
        if self._run_number:
            instr_name  = self._instr_name()
            name= name.replace(instr_name,'',1)
            self._ws_cname = filter(lambda c: not c.isdigit(), name)

        else:
            self._ws_cname = name
    def _instr_name(self):
       if RunDescriptor._holder:
            instr_name = RunDescriptor._holder.short_inst_name
       else:
            instr_name = '_test_instrument'
       return instr_name 

    def _clear_old_ws(self,old_ws_name,new_name,clear_runs_to_add=True):
        """ helper method used in __set__. When new data (run or wod)  """
        if old_ws_name:
           if new_name != old_ws_name:
                if old_ws_name in mtd:
                   DeleteWorkspace(old_ws_name)
                old_mon_ws = old_ws_name+'_monitors'
                if old_mon_ws  in mtd:
                    DeleteWorkspace(old_mon_ws)

                self._run_ext     = None
                self._run_file_path = ''
                if clear_runs_to_add:
                    self._runs_to_add = None


#-------------------------------------------------------------------------------------------------------------------------------
#-------------------------------------------------------------------------------------------------------------------------------  
#-------------------------------------------------------------------------------------------------------------------------------
class RunDescriptorDependent(RunDescriptor):
    """ Simple RunDescriptor class dependent on another RunDescriptor,
        providing the host descriptor if current descriptor value is not defined
        or usual descriptor functionality if somebody sets current descriptor up   
    """

    def __init__(self,host_run,ws_preffix,DocString=None):
        RunDescriptor.__init__(self,ws_preffix,DocString)
        self._host = host_run
        self._this_run_defined=False

    def __get__(self,instance,owner=None):
       """ return dependent run number which is host run number if this one has not been set or this run number if it was""" 
       if self._this_run_defined:
             if instance is None:
                 return self
             else:
                return super(RunDescriptorDependent,self).__get__(instance)
       else:
          return self._host.__get__(instance,owner)

    def __set__(self,instance,value):
        if value is None:
            self._this_run_defined = False
            return
        self._this_run_defined = True
        super(RunDescriptorDependent,self).__set__(instance,value)

    #def __del__(self):
    #    # destructor removes bounded workspace 
    #    # Probably better not to at current approach
    #    if self._ws_name in mtd:
    #        DeleteWorkspace(self._ws_name)
    #    object.__del__(self)

  