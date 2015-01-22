from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode
from mantid.kernel import StringMandatoryValidator, Direction, logger


class IndirectCylinderAbsorption(DataProcessorAlgorithm):

    def category(self):
        return "Workflow\\Inelastic;PythonAlgorithms;CorrectionFunctions\\AbsorptionCorrections"


    def summary(self):
        return "Calculates indirect absorption corrections for a cylinder sample shape."


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Sample workspace.')

        self.declareProperty(MatrixWorkspaceProperty('CanWorkspace', '', optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc='Container workspace.')

        self.declareProperty(name='CanScaleFactor', defaultValue=1.0, doc='Scale factor to multiply can data')

        self.declareProperty(name='ChemicalFormula', defaultValue='', validator=StringMandatoryValidator(),
                             doc='Chemical formula')

        self.declareProperty(name='SampleRadius', defaultValue=0.0, doc='Sample radius')
        self.declareProperty(name='SampleNumberDensity', defaultValue=0.1, doc='Sample number density')
        self.declareProperty(name='Plot', defaultValue=False, doc='Plot options')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output corrected workspace.')

        self.declareProperty(MatrixWorkspaceProperty('CorrectionsWorkspace', '', direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The corrections workspace for scattering and absorptions in sample.')


    def PyExec(self):
        from IndirectCommon import getEfixed, addSampleLogs

        self._setup()
        efixed = getEfixed(self._sample_ws)

        sample_wave_ws = '__sam_wave'
        ConvertUnits(InputWorkspace=self._sample_ws, OutputWorkspace=sample_wave_ws,
                     Target='Wavelength', EMode='Indirect', EFixed=efixed)

        SetSampleMaterial(sample_wave_ws, ChemicalFormula=self._chemical_formula, SampleNumberDensity=self._number_density)

        CylinderAbsorption(InputWorkspace=sample_wave_ws,
                           OutputWorkspace=self._ass_ws,
                           SampleNumberDensity=self._number_density,
                           NumberOfWavelengthPoints=10,
                           CylinderSampleHeight=3.0,
                           CylinderSampleRadius=self._sample_radius,
                           NumberOfSlices=1,
                           NumberOfAnnuli=10)

        plot_list = [self._output_ws, self._sample_ws]

        if self._can_ws is not None:
            can_wave_ws = '__can_wave'
            ConvertUnits(InputWorkspace=self._can_ws, OutputWorkspace=can_wave_ws,
                         Target='Wavelength', EMode='Indirect', EFixed=efixed)

            if self._can_scale != 1.0:
                logger.information('Scaling can by: ' + str(self._can_scale))
                Scale(InputWorkspace=can_wave_ws, OutputWorkspace=can_wave_ws, Factor=self._can_scale, Operation='Multiply')

            Minus(LHSWorkspace=sample_wave_ws, RHSWorkspace=can_wave_ws, OutputWorkspace=sample_wave_ws)
            DeleteWorkspace(can_wave_ws)

            plot_list.append(self._can_ws)

        Divide(LHSWorkspace=sample_wave_ws, RHSWorkspace=self._ass_ws, OutputWorkspace=sample_wave_ws)
        ConvertUnits(InputWorkspace=sample_wave_ws, OutputWorkspace=self._output_ws,
                     Target='DeltaE', EMode='Indirect', EFixed=efixed)
        DeleteWorkspace(sample_wave_ws)

        sample_logs = {'sample_shape': 'cylinder',
                       'sample_filename': self._sample_ws,
                       'sample_radius': self._sample_radius}
        addSampleLogs(self._ass_ws, sample_logs)
        addSampleLogs(self._output_ws, sample_logs)

        if self._can_ws is not None:
            AddSampleLog(Workspace=self._ass_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws))
            AddSampleLog(Workspace=self._output_ws, LogName='can_filename', LogType='String', LogText=str(self._can_ws))
            AddSampleLog(Workspace=self._ass_ws, LogName='can_scale', LogType='String', LogText=str(self._can_scale))
            AddSampleLog(Workspace=self._output_ws, LogName='can_scale', LogType='String', LogText=str(self._can_scale))

        self.setProperty('OutputWorkspace', self._output_ws)

        # Output the Ass workspace if it is wanted, delete if not
        if self._ass_ws == '_ass':
            DeleteWorkspace(self._ass_ws)
        else:
            self.setProperty('CorrectionsWorkspace', self._ass_ws)

        if self._plot:
            from IndirectImport import import_mantidplot
            mantid_plot = import_mantidplot()
            mantid_plot.plotSpectrum(plot_list, 0)


    def _setup(self):
        """
        Get algorithm properties.
        """

        self._sample_ws = self.getPropertyValue('SampleWorkspace')
        self._can_scale = self.getProperty('CanScaleFactor').value
        self._chemical_formula = self.getPropertyValue('ChemicalFormula')
        self._number_density = self.getProperty('SampleNumberDensity').value
        self._sample_radius = self.getProperty('SampleRadius').value
        self._plot = self.getProperty('Plot').value
        self._output_ws = self.getPropertyValue('OutputWorkspace')

        self._ass_ws = self.getPropertyValue('CorrectionsWorkspace')
        if self._ass_ws == '':
            self._ass_ws = '__ass'

        self._can_ws = self.getPropertyValue('CanWorkspace')
        if self._can_ws == '':
            self._can_ws = None


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectCylinderAbsorption)
