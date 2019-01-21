package com.catramms.backing.workflowEditor.Properties;

import com.catramms.backing.newWorkflow.WorkflowIssue;
import com.catramms.backing.workflowEditor.utility.IngestionData;
import org.apache.log4j.Logger;
import org.json.JSONArray;
import org.json.JSONObject;

import java.io.Serializable;
import java.util.List;

public class CutProperties extends CreateContentProperties implements Serializable {

    private static final Logger mLogger = Logger.getLogger(CutProperties.class);

    private Long timeInSecondsDecimalsPrecision;
    private Float startTimeInSeconds;
    private String endType;
    private Float endTimeInSeconds;
    private Long framesNumber;
    private String fileFormat;

    private StringBuilder taskReferences = new StringBuilder();

    public CutProperties(int elementId, String label)
    {
        super(elementId, label, "Cut" + "-icon.png", "Task", "Cut");

        endType = "endTime";
        timeInSecondsDecimalsPrecision = new Long(6);
    }

    public CutProperties clone()
    {
        CutProperties cutProperties = new CutProperties(
                super.getElementId(), super.getLabel());

        cutProperties.setTitle(getTitle());
        cutProperties.setTags(getTags());
        cutProperties.setRetention(getRetention());
        cutProperties.setStartPublishing(getStartPublishing());
        cutProperties.setEndPublishing(getEndPublishing());
        cutProperties.setUserData(getUserData());
        cutProperties.setIngester(getIngester());
        cutProperties.setContentProviderName(getContentProviderName());
        cutProperties.setDeliveryFileName(getDeliveryFileName());
        cutProperties.setUniqueName(getUniqueName());

        cutProperties.setStartTimeInSeconds(startTimeInSeconds);
        cutProperties.setEndType(endType);
        cutProperties.setEndTimeInSeconds(endTimeInSeconds);
        cutProperties.setFramesNumber(framesNumber);
        cutProperties.setFileFormat(fileFormat);

        cutProperties.setStringBuilderTaskReferences(taskReferences);

        return cutProperties;
    }

    public JSONObject buildWorkflowElementJson(IngestionData ingestionData)
            throws Exception
    {
        JSONObject jsonWorkflowElement = new JSONObject();

        try
        {
            jsonWorkflowElement.put("Type", super.getType());

            JSONObject joParameters = new JSONObject();
            jsonWorkflowElement.put("Parameters", joParameters);

            mLogger.info("task.getType: " + super.getType());

            if (super.getLabel() != null && !super.getLabel().equalsIgnoreCase(""))
                jsonWorkflowElement.put("Label", super.getLabel());
            else
            {
                WorkflowIssue workflowIssue = new WorkflowIssue();
                workflowIssue.setLabel("");
                workflowIssue.setFieldName("Label");
                workflowIssue.setTaskType(super.getType());
                workflowIssue.setIssue("The field is not initialized");

                ingestionData.getWorkflowIssueList().add(workflowIssue);
            }

            if (getStartTimeInSeconds() != null)
                joParameters.put("StartTimeInSeconds", getStartTimeInSeconds());
                // String.format("%." + timeInSecondsDecimalsPrecision + "g",
                //        task.getStartTimeInSeconds().floatValue()));
            else
            {
                WorkflowIssue workflowIssue = new WorkflowIssue();
                workflowIssue.setLabel(getLabel());
                workflowIssue.setFieldName("StartTimeInSeconds");
                workflowIssue.setTaskType(getType());
                workflowIssue.setIssue("The field is not initialized");

                ingestionData.getWorkflowIssueList().add(workflowIssue);
            }
            if (getEndType().equalsIgnoreCase("endTime"))
            {
                if (getEndTimeInSeconds() != null)
                    joParameters.put("EndTimeInSeconds", getEndTimeInSeconds());
                    // String.format("%." + timeInSecondsDecimalsPrecision + "g",
                    //        task.getEndTimeInSeconds().floatValue()));
                else
                {
                    WorkflowIssue workflowIssue = new WorkflowIssue();
                    workflowIssue.setLabel(getLabel());
                    workflowIssue.setFieldName("EndTimeInSeconds");
                    workflowIssue.setTaskType(getType());
                    workflowIssue.setIssue("The field is not initialized");

                    ingestionData.getWorkflowIssueList().add(workflowIssue);
                }
            }
            else
            {
                if (getFramesNumber() != null)
                    joParameters.put("FramesNumber", getFramesNumber());
                else
                {
                    WorkflowIssue workflowIssue = new WorkflowIssue();
                    workflowIssue.setLabel(getLabel());
                    workflowIssue.setFieldName("FramesNumber");
                    workflowIssue.setTaskType(getType());
                    workflowIssue.setIssue("The field is not initialized");

                    ingestionData.getWorkflowIssueList().add(workflowIssue);
                }
            }
            if (getFileFormat() != null && !getFileFormat().equalsIgnoreCase(""))
                joParameters.put("OutputFileFormat", getFileFormat());

            super.addCreateContentPropertiesToJson(joParameters);

            if (taskReferences != null && !taskReferences.toString().equalsIgnoreCase(""))
            {
                JSONArray jaReferences = new JSONArray();
                joParameters.put("References", jaReferences);

                String [] physicalPathKeyReferences = taskReferences.toString().split(",");
                for (String physicalPathKeyReference: physicalPathKeyReferences)
                {
                    JSONObject joReference = new JSONObject();
                    joReference.put("ReferencePhysicalPathKey", Long.parseLong(physicalPathKeyReference.trim()));

                    jaReferences.put(joReference);
                }
            }

            super.addEventsPropertiesToJson(jsonWorkflowElement, ingestionData);
        }
        catch (Exception e)
        {
            mLogger.error("buildWorkflowJson failed: " + e);

            throw e;
        }

        return jsonWorkflowElement;
    }

    public Float getStartTimeInSeconds() {
        return startTimeInSeconds;
    }

    public void setStartTimeInSeconds(Float startTimeInSeconds) {
        this.startTimeInSeconds = startTimeInSeconds;
    }

    public Float getEndTimeInSeconds() {
        return endTimeInSeconds;
    }

    public void setEndTimeInSeconds(Float endTimeInSeconds) {
        this.endTimeInSeconds = endTimeInSeconds;
    }

    public Long getFramesNumber() {
        return framesNumber;
    }

    public void setFramesNumber(Long framesNumber) {
        this.framesNumber = framesNumber;
    }

    public String getEndType() {
        return endType;
    }

    public void setEndType(String endType) {
        this.endType = endType;
    }

    public String getFileFormat() {
        return fileFormat;
    }

    public void setFileFormat(String fileFormat) {
        this.fileFormat = fileFormat;
    }

    public void setStringBuilderTaskReferences(StringBuilder taskReferences) {
        this.taskReferences = taskReferences;
    }

    public StringBuilder getStringBuilderTaskReferences() {
        return taskReferences;
    }

    public String getTaskReferences() {
        return taskReferences.toString();
    }

    public void setTaskReferences(String taskReferences) {
        this.taskReferences.replace(0, this.taskReferences.length(), taskReferences);
    }

    public Long getTimeInSecondsDecimalsPrecision() {
        return timeInSecondsDecimalsPrecision;
    }

    public void setTimeInSecondsDecimalsPrecision(Long timeInSecondsDecimalsPrecision) {
        this.timeInSecondsDecimalsPrecision = timeInSecondsDecimalsPrecision;
    }
}