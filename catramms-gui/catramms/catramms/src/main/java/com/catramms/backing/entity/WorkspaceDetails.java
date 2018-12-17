package com.catramms.backing.entity;

import java.io.Serializable;
import java.util.Date;

/**
 * Created by multi on 08.06.18.
 */
public class WorkspaceDetails implements Serializable {
    private Long workspaceKey;
    private Boolean isEnabled;
    private String name;
    private String maxEncodingPriority;
    private String encodingPeriod;
    private Long maxIngestionsNumber;
    private Long maxStorageInMB;
    private String languageCode;
    private Date creationDate;
    private String apiKey;
    private Boolean owner;
    private Boolean admin;
    private Boolean ingestWorkflow;
    private Boolean createProfiles;
    private Boolean deliveryAuthorization;
    private Boolean shareWorkspace;
    private Boolean editMedia;

    public Long getWorkspaceKey() {
        return workspaceKey;
    }

    public void setWorkspaceKey(Long workspaceKey) {
        this.workspaceKey = workspaceKey;
    }

    public Boolean getOwner() {
        return owner;
    }

    public void setOwner(Boolean owner) {
        this.owner = owner;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getApiKey() {
        return apiKey;
    }

    public void setApiKey(String apiKey) {
        this.apiKey = apiKey;
    }

    public Boolean getAdmin() {
        return admin;
    }

    public void setAdmin(Boolean admin) {
        this.admin = admin;
    }

    public Boolean getIngestWorkflow() {
        return ingestWorkflow;
    }

    public void setIngestWorkflow(Boolean ingestWorkflow) {
        this.ingestWorkflow = ingestWorkflow;
    }

    public Boolean getCreateProfiles() {
        return createProfiles;
    }

    public void setCreateProfiles(Boolean createProfiles) {
        this.createProfiles = createProfiles;
    }

    public Boolean getDeliveryAuthorization() {
        return deliveryAuthorization;
    }

    public void setDeliveryAuthorization(Boolean deliveryAuthorization) {
        this.deliveryAuthorization = deliveryAuthorization;
    }

    public Boolean getShareWorkspace() {
        return shareWorkspace;
    }

    public void setShareWorkspace(Boolean shareWorkspace) {
        this.shareWorkspace = shareWorkspace;
    }

    public Boolean getEditMedia() {
        return editMedia;
    }

    public void setEditMedia(Boolean editMedia) {
        this.editMedia = editMedia;
    }

    public Boolean getEnabled() {
        return isEnabled;
    }

    public void setEnabled(Boolean enabled) {
        isEnabled = enabled;
    }

    public String getMaxEncodingPriority() {
        return maxEncodingPriority;
    }

    public void setMaxEncodingPriority(String maxEncodingPriority) {
        this.maxEncodingPriority = maxEncodingPriority;
    }

    public String getEncodingPeriod() {
        return encodingPeriod;
    }

    public void setEncodingPeriod(String encodingPeriod) {
        this.encodingPeriod = encodingPeriod;
    }

    public Long getMaxIngestionsNumber() {
        return maxIngestionsNumber;
    }

    public void setMaxIngestionsNumber(Long maxIngestionsNumber) {
        this.maxIngestionsNumber = maxIngestionsNumber;
    }

    public Long getMaxStorageInMB() {
        return maxStorageInMB;
    }

    public void setMaxStorageInMB(Long maxStorageInMB) {
        this.maxStorageInMB = maxStorageInMB;
    }

    public String getLanguageCode() {
        return languageCode;
    }

    public void setLanguageCode(String languageCode) {
        this.languageCode = languageCode;
    }

    public Date getCreationDate() {
        return creationDate;
    }

    public void setCreationDate(Date creationDate) {
        this.creationDate = creationDate;
    }
}
