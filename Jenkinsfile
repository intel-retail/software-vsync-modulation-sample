// 1cicd: ["intel-innersource/applications.devops.jenkins.jenkins-common-pipelines"]

pipeline {
    agent { label 'docker' }
    options {
        disableConcurrentBuilds()
        buildDiscarder(logRotator(daysToKeepStr: '30'))
    }
    environment {
        BDBA_BIN_DIR = 'binaries'
    }
    stages {
        stage('Build') {
            agent {
               docker {
                   image 'ubuntu:22.04'
	       }
            }
	    stages{
		    stage('Build') {
			steps {
			    sh '''			    
			    rm -rf release
			    apt update && apt -qq install -y libdrm-dev libpciaccess-dev build-essential
			    make release
			    mkdir -p binaries
			    mv release/* ${BDBA_BIN_DIR}/
			    '''
			}
		    }
	    }
        }
        stage('Scan') {
            environment {
                PROJECT_NAME = 'software-vsync-modulation'
                        SNYK_PROJECT_NAME = ''
                        BANDIT_SOURCE_PATH   = '.'
                SNYK_MANIFEST_FILE = ''
                SCANNERS = 'checkmarx, coverity, bdba, virus'
                    BDBA_INCLUDE_SUB_DIRS = true
                COVERITY_PROJECT_NAME = 'Software Vsync Modulation Sample'
                COVERITY_PROJECT_VERSION = 'v1.0.0' // this is not the real version, just testing for report generation
                COVERITY_PREBUILD_SCRIPT = 'apt update && apt -qq install -y libdrm-dev libpciaccess-dev build-essential'
                COVERITY_BUILD_SCRIPT = 'make release'
                COVERITY_CHECKER_CONFIG = '--concurrency --security --rule --enable-constraint-fpp --enable-fnptr --enable-virtual'
                COVERITY_COMPILER = 'c++'
                COVERITY_COMPILER_TYPE = 'gcc'
                SDLE_UPLOAD_PROJECT_ID = '20401'
            }
            steps {
                rbheStaticCodeScan()
            }
        }
    }
    post {
        always {
            jcpSummaryReport()
        }
    }
}
